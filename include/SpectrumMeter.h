#pragma once

#include "Config.h"
#include "AudioCapture.h"
#include "FFTAnalyzer.h"
#include "Renderer.h"
#include <memory>
#include <atomic>

class SpectrumMeter {
public:
    SpectrumMeter(const Config& config);
    ~SpectrumMeter();

    bool initialize();
    void run();
    void shutdown();

private:
    void onAudioData(const float* samples, size_t count);

    Config config;

    std::unique_ptr<AudioCapture> audioCapture;
    std::unique_ptr<FFTAnalyzer> fftAnalyzer;
    std::unique_ptr<Renderer> renderer;

    std::atomic<bool> running{false};
};
