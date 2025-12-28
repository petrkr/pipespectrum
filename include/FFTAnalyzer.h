#pragma once

#include <fftw3.h>
#include <vector>
#include <mutex>

class FFTAnalyzer {
public:
    FFTAnalyzer(int fftSize, int sampleRate, int numBands, float minFreq, float maxFreq,
                float minDb = -80.0f, float maxDb = 0.0f, float noiseThreshold = 0.05f,
                bool freqWeighting = true, float smoothing = 0.3f);
    ~FFTAnalyzer();

    void process(const float* samples, size_t count);

    const std::vector<float>& getBands() const { return bands; }
    const std::vector<float>& getPeaks() const { return peaks; }

    void updatePeaks(float decayAmount);

private:
    void performFFT();
    void calculateBands();
    int freqToFFTBin(float freq) const;
    float getFrequencyWeight(float freq) const;

    int fftSize;
    int sampleRate;
    int numBands;
    float minFreq;
    float maxFreq;
    float minDb;
    float maxDb;
    float noiseThreshold;
    bool freqWeighting;
    float smoothing;

    std::vector<float> inputBuffer;
    std::vector<float> windowFunction;
    size_t bufferPos = 0;

    fftwf_complex* fftOutput;
    fftwf_plan fftPlan;

    std::vector<float> bands;
    std::vector<float> peaks;
    std::vector<float> smoothedBands;

    mutable std::mutex mutex;
};
