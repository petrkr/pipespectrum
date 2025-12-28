#pragma once

#include <string>
#include <array>
#include <cstdint>

struct WindowConfig {
    std::string title = "PipeSpectrum";
    int width = 1280;
    int height = 720;
    bool vsync = true;
};

struct SpectrumConfig {
    int bands = 64;
    float minFreq = 20.0f;
    float maxFreq = 20000.0f;
    int fftSize = 8192;
    int sampleRate = 48000;
    float smoothing = 0.7f;
    bool peakHoldEnabled = true;
    float peakFallTime = 1.5f;  // seconds
    float minDb = -80.0f;
    float maxDb = 0.0f;
    float noiseThreshold = 0.05f;
    bool freqWeighting = true;
};

struct VisualizationConfig {
    std::array<uint8_t, 3> barColorLow = {0, 255, 0};
    std::array<uint8_t, 3> barColorMid = {255, 255, 0};
    std::array<uint8_t, 3> barColorHigh = {255, 0, 0};
    std::array<uint8_t, 3> peakColor = {255, 255, 255};
    int barGap = 2;
    bool barGradient = true;
};

struct AudioConfig {
    int targetLatency = 20;
    int bufferSize = 1024;
};

class Config {
public:
    Config();
    bool load(const std::string& filename);

    const WindowConfig& getWindow() const { return window; }
    const SpectrumConfig& getSpectrum() const { return spectrum; }
    const VisualizationConfig& getVisualization() const { return visualization; }
    const AudioConfig& getAudio() const { return audio; }

private:
    WindowConfig window;
    SpectrumConfig spectrum;
    VisualizationConfig visualization;
    AudioConfig audio;
};
