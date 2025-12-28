#include "SpectrumMeter.h"
#include <iostream>
#include <thread>
#include <chrono>

SpectrumMeter::SpectrumMeter(const Config& config)
    : config(config) {
}

SpectrumMeter::~SpectrumMeter() {
    shutdown();
}

bool SpectrumMeter::initialize() {
    const auto& specConfig = config.getSpectrum();
    const auto& audioConfig = config.getAudio();
    const auto& windowConfig = config.getWindow();
    const auto& visConfig = config.getVisualization();

    // Create FFT analyzer
    fftAnalyzer = std::make_unique<FFTAnalyzer>(
        specConfig.fftSize,
        specConfig.sampleRate,
        specConfig.bands,
        specConfig.minFreq,
        specConfig.maxFreq,
        specConfig.minDb,
        specConfig.maxDb,
        specConfig.noiseThreshold,
        specConfig.freqWeighting,
        specConfig.smoothing
    );

    // Create audio capture
    audioCapture = std::make_unique<AudioCapture>(
        specConfig.sampleRate,
        audioConfig.bufferSize
    );

    // Set audio callback
    audioCapture->setCallback([this](const float* samples, size_t count) {
        onAudioData(samples, count);
    });

    if (!audioCapture->initialize()) {
        std::cerr << "Failed to initialize audio capture" << std::endl;
        return false;
    }

    // Create renderer
    renderer = std::make_unique<Renderer>(windowConfig, visConfig);

    if (!renderer->initialize()) {
        std::cerr << "Failed to initialize renderer" << std::endl;
        return false;
    }

    std::cout << "PipeSpectrum initialized successfully" << std::endl;
    return true;
}

void SpectrumMeter::run() {
    running.store(true);
    audioCapture->start();

    const auto& specConfig = config.getSpectrum();

    // Main loop
    using clock = std::chrono::high_resolution_clock;
    auto lastFrame = clock::now();
    const auto targetFrameTime = std::chrono::milliseconds(16); // ~60 FPS

    while (running.load() && !renderer->shouldClose()) {
        auto frameStart = clock::now();

        // Poll events
        renderer->pollEvents();

        // Update peaks (linear decay for consistent fall time)
        if (specConfig.peakHoldEnabled) {
            // Linear decay: fall from 1.0 to 0.0 in exactly N seconds
            // At 60 FPS: decay_amount = 1.0 / (fall_time * 60)
            float framesPerSecond = 60.0f;
            float decayAmount = 1.0f / (specConfig.peakFallTime * framesPerSecond);
            fftAnalyzer->updatePeaks(decayAmount);
        }

        // Render
        renderer->clear();
        renderer->renderSpectrum(
            fftAnalyzer->getBands(),
            fftAnalyzer->getPeaks(),
            specConfig.peakHoldEnabled
        );
        renderer->present();

        // Frame timing
        auto frameEnd = clock::now();
        auto frameTime = frameEnd - frameStart;

        if (frameTime < targetFrameTime) {
            std::this_thread::sleep_for(targetFrameTime - frameTime);
        }

        lastFrame = frameStart;
    }

    std::cout << "Main loop exited" << std::endl;
}

void SpectrumMeter::shutdown() {
    running.store(false);

    if (audioCapture) {
        audioCapture->stop();
    }

    std::cout << "PipeSpectrum shutdown" << std::endl;
}

void SpectrumMeter::onAudioData(const float* samples, size_t count) {
    if (fftAnalyzer) {
        fftAnalyzer->process(samples, count);
    }
}
