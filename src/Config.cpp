#include "Config.h"
#include <yaml-cpp/yaml.h>
#include <iostream>

Config::Config() = default;

bool Config::load(const std::string& filename) {
    try {
        YAML::Node config = YAML::LoadFile(filename);

        // Window config
        if (config["window"]) {
            auto win = config["window"];
            if (win["title"]) window.title = win["title"].as<std::string>();
            if (win["width"]) window.width = win["width"].as<int>();
            if (win["height"]) window.height = win["height"].as<int>();
            if (win["vsync"]) window.vsync = win["vsync"].as<bool>();
        }

        // Spectrum config
        if (config["spectrum"]) {
            auto spec = config["spectrum"];
            if (spec["bands"]) spectrum.bands = spec["bands"].as<int>();
            if (spec["min_freq"]) spectrum.minFreq = spec["min_freq"].as<float>();
            if (spec["max_freq"]) spectrum.maxFreq = spec["max_freq"].as<float>();
            if (spec["fft_size"]) spectrum.fftSize = spec["fft_size"].as<int>();
            if (spec["sample_rate"]) spectrum.sampleRate = spec["sample_rate"].as<int>();
            if (spec["smoothing"]) spectrum.smoothing = spec["smoothing"].as<float>();
            if (spec["peak_hold_enabled"]) spectrum.peakHoldEnabled = spec["peak_hold_enabled"].as<bool>();
            if (spec["peak_fall_time"]) spectrum.peakFallTime = spec["peak_fall_time"].as<float>();
            if (spec["min_db"]) spectrum.minDb = spec["min_db"].as<float>();
            if (spec["max_db"]) spectrum.maxDb = spec["max_db"].as<float>();
            if (spec["noise_threshold"]) spectrum.noiseThreshold = spec["noise_threshold"].as<float>();
            if (spec["freq_weighting"]) spectrum.freqWeighting = spec["freq_weighting"].as<bool>();
        }

        // Visualization config
        if (config["visualization"]) {
            auto vis = config["visualization"];
            if (vis["bar_color_low"]) {
                auto color = vis["bar_color_low"].as<std::vector<int>>();
                for (size_t i = 0; i < 3 && i < color.size(); ++i)
                    visualization.barColorLow[i] = static_cast<uint8_t>(color[i]);
            }
            if (vis["bar_color_mid"]) {
                auto color = vis["bar_color_mid"].as<std::vector<int>>();
                for (size_t i = 0; i < 3 && i < color.size(); ++i)
                    visualization.barColorMid[i] = static_cast<uint8_t>(color[i]);
            }
            if (vis["bar_color_high"]) {
                auto color = vis["bar_color_high"].as<std::vector<int>>();
                for (size_t i = 0; i < 3 && i < color.size(); ++i)
                    visualization.barColorHigh[i] = static_cast<uint8_t>(color[i]);
            }
            if (vis["peak_color"]) {
                auto color = vis["peak_color"].as<std::vector<int>>();
                for (size_t i = 0; i < 3 && i < color.size(); ++i)
                    visualization.peakColor[i] = static_cast<uint8_t>(color[i]);
            }
            if (vis["bar_gap"]) visualization.barGap = vis["bar_gap"].as<int>();
            if (vis["bar_gradient"]) visualization.barGradient = vis["bar_gradient"].as<bool>();
        }

        // Audio config
        if (config["audio"]) {
            auto aud = config["audio"];
            if (aud["target_latency"]) audio.targetLatency = aud["target_latency"].as<int>();
            if (aud["buffer_size"]) audio.bufferSize = aud["buffer_size"].as<int>();
        }

        return true;
    } catch (const YAML::Exception& e) {
        std::cerr << "Error loading config: " << e.what() << std::endl;
        return false;
    }
}
