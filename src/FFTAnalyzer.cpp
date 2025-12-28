#include "FFTAnalyzer.h"
#include <cmath>
#include <algorithm>
#include <iostream>

FFTAnalyzer::FFTAnalyzer(int fftSize, int sampleRate, int numBands, float minFreq, float maxFreq,
                         float minDb, float maxDb, float noiseThreshold, bool freqWeighting, float smoothing)
    : fftSize(fftSize), sampleRate(sampleRate), numBands(numBands),
      minFreq(minFreq), maxFreq(maxFreq), minDb(minDb), maxDb(maxDb),
      noiseThreshold(noiseThreshold), freqWeighting(freqWeighting), smoothing(smoothing) {

    inputBuffer.resize(fftSize, 0.0f);
    windowFunction.resize(fftSize);
    bands.resize(numBands, 0.0f);
    peaks.resize(numBands, 0.0f);
    smoothedBands.resize(numBands, 0.0f);

    // Hanning window
    for (int i = 0; i < fftSize; ++i) {
        windowFunction[i] = 0.5f * (1.0f - std::cos(2.0f * M_PI * i / (fftSize - 1)));
    }

    // Allocate FFTW buffers
    fftOutput = fftwf_alloc_complex(fftSize / 2 + 1);
    fftPlan = fftwf_plan_dft_r2c_1d(fftSize, inputBuffer.data(), fftOutput, FFTW_MEASURE);

    std::cout << "FFT Analyzer initialized: " << numBands << " bands, "
              << fftSize << " FFT size" << std::endl;
}

FFTAnalyzer::~FFTAnalyzer() {
    fftwf_destroy_plan(fftPlan);
    fftwf_free(fftOutput);
}

void FFTAnalyzer::process(const float* samples, size_t count) {
    std::lock_guard<std::mutex> lock(mutex);

    static int processCount = 0;
    static float maxSample = 0.0f;

    // Convert stereo to mono and accumulate in buffer
    for (size_t i = 0; i < count; i += 2) {
        if (bufferPos < static_cast<size_t>(fftSize)) {
            // Average left and right channels
            float sample = (samples[i] + samples[i + 1]) * 0.5f;
            inputBuffer[bufferPos++] = sample;
            maxSample = std::max(maxSample, std::abs(sample));
        }

        // When buffer is full, perform FFT
        if (bufferPos >= static_cast<size_t>(fftSize)) {
            if (++processCount % 10 == 0) {
                std::cout << "[FFT] Processing FFT #" << processCount
                          << ", max sample: " << maxSample << std::endl;
                maxSample = 0.0f;
            }

            performFFT();
            calculateBands();

            // Overlap: shift buffer by half
            std::copy(inputBuffer.begin() + fftSize / 2, inputBuffer.end(),
                     inputBuffer.begin());
            bufferPos = fftSize / 2;
        }
    }
}

void FFTAnalyzer::performFFT() {
    // Apply window function
    for (int i = 0; i < fftSize; ++i) {
        inputBuffer[i] *= windowFunction[i];
    }

    // Execute FFT
    fftwf_execute(fftPlan);
}

void FFTAnalyzer::calculateBands() {
    bands.assign(numBands, 0.0f);

    static int calcCount = 0;
    static float maxBand = 0.0f;

    // Logarithmic frequency distribution
    float logMin = std::log10(minFreq);
    float logMax = std::log10(maxFreq);
    float logStep = (logMax - logMin) / numBands;

    for (int band = 0; band < numBands; ++band) {
        float freqLow = std::pow(10.0f, logMin + band * logStep);
        float freqHigh = std::pow(10.0f, logMin + (band + 1) * logStep);

        int binLow = freqToFFTBin(freqLow);
        int binHigh = freqToFFTBin(freqHigh);

        // Average magnitude in this frequency range
        float sum = 0.0f;
        int count = 0;

        for (int bin = binLow; bin <= binHigh && bin < fftSize / 2; ++bin) {
            float real = fftOutput[bin][0];
            float imag = fftOutput[bin][1];
            float magnitude = std::sqrt(real * real + imag * imag);
            sum += magnitude;
            count++;
        }

        if (count > 0) {
            float average = sum / count;

            // Normalize by FFT size
            average = average / (fftSize * 0.5f);

            // Apply frequency weighting (boost high frequencies)
            if (freqWeighting) {
                float centerFreq = std::sqrt(freqLow * freqHigh);
                float weight = getFrequencyWeight(centerFreq);
                average *= weight;
            }

            // Apply sensitivity/gain and convert to dB
            const float sensitivity = 2.0f;
            float db = 20.0f * std::log10(average * sensitivity + 1e-9f);

            // Map dB range to 0-1 using config values
            float dbRange = maxDb - minDb;
            float normalized = std::max(0.0f, (db - minDb) / dbRange);
            normalized = std::min(1.0f, normalized);

            // Apply noise gate
            if (normalized < noiseThreshold) {
                normalized = 0.0f;
            }

            bands[band] = normalized;
        }
    }

    // Smooth bands (use config value)
    for (int i = 0; i < numBands; ++i) {
        smoothedBands[i] = smoothedBands[i] * smoothing + bands[i] * (1.0f - smoothing);
        bands[i] = smoothedBands[i];
        maxBand = std::max(maxBand, bands[i]);
    }

    if (++calcCount % 10 == 0) {
        std::cout << "[FFT] Bands calculated #" << calcCount
                  << ", max band value: " << maxBand << std::endl;
        maxBand = 0.0f;
    }
}

void FFTAnalyzer::updatePeaks(float decayAmount) {
    std::lock_guard<std::mutex> lock(mutex);

    for (int i = 0; i < numBands; ++i) {
        if (bands[i] > peaks[i]) {
            peaks[i] = bands[i];
        } else {
            // Linear decay - constant fall rate
            peaks[i] -= decayAmount;
            if (peaks[i] < 0.0f) peaks[i] = 0.0f;
        }
    }
}

int FFTAnalyzer::freqToFFTBin(float freq) const {
    return static_cast<int>(freq * fftSize / sampleRate);
}

float FFTAnalyzer::getFrequencyWeight(float freq) const {
    // Simplified A-weighting style curve
    // Boosts high frequencies to compensate for natural rolloff
    if (freq < 1000.0f) {
        // Low frequencies: gradual boost from 20Hz to 1kHz
        return 1.0f + (freq - 20.0f) / 1000.0f * 0.5f;
    } else {
        // High frequencies: significant boost above 1kHz
        float octaves = std::log2(freq / 1000.0f);
        return 1.5f + octaves * 0.8f; // +0.8x per octave above 1kHz
    }
}
