#include "SpectrumMeter.h"
#include "Config.h"
#include <iostream>
#include <signal.h>
#include <filesystem>
#include <cstdlib>

static SpectrumMeter* g_spectrumMeter = nullptr;

void signalHandler(int signal) {
    std::cout << "\nReceived signal " << signal << ", shutting down..." << std::endl;
    if (g_spectrumMeter) {
        g_spectrumMeter->shutdown();
    }
}

int main(int argc, char* argv[]) {
    std::cout << "=== PipeSpectrum - FFT Spectrum Analyzer ===" << std::endl;
    std::cout << "Press ESC or Q to quit" << std::endl << std::endl;

    // Load configuration
    Config config;
    std::string configFile;

    if (argc > 1) {
        // Use config file from command line argument
        configFile = argv[1];
    } else {
        // Use user config in ~/.config/pipespectrum/config.yml
        const char* home = std::getenv("HOME");
        if (home) {
            std::filesystem::path configDir = std::filesystem::path(home) / ".config" / "pipespectrum";
            std::filesystem::path userConfig = configDir / "config.yml";
            configFile = userConfig;

            // Create config directory if it doesn't exist
            if (!std::filesystem::exists(configDir)) {
                std::filesystem::create_directories(configDir);
                std::cout << "Created config directory: " << configDir << std::endl;
            }

            // Copy sample config if user config doesn't exist
            if (!std::filesystem::exists(userConfig)) {
                // Try to find sample config in standard installation paths
                std::vector<std::filesystem::path> samplePaths = {
                    "/usr/share/pipespectrum/config.sample.yaml",
                    "/usr/local/share/pipespectrum/config.sample.yaml",
                    "../config.sample.yaml",  // For running from build directory
                    "config.sample.yaml"
                };

                for (const auto& samplePath : samplePaths) {
                    if (std::filesystem::exists(samplePath)) {
                        try {
                            std::filesystem::copy_file(samplePath, userConfig);
                            std::cout << "Created config file from sample: " << userConfig << std::endl;
                            break;
                        } catch (const std::filesystem::filesystem_error& e) {
                            std::cerr << "Failed to copy config: " << e.what() << std::endl;
                        }
                    }
                }
            }
        } else {
            configFile = "config.yml";
        }
    }

    if (!config.load(configFile)) {
        std::cerr << "Warning: Failed to load config file '" << configFile
                  << "', using defaults" << std::endl;
    }

    // Create spectrum meter
    SpectrumMeter spectrumMeter(config);
    g_spectrumMeter = &spectrumMeter;

    // Signal handlers
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);

    // Initialize
    if (!spectrumMeter.initialize()) {
        std::cerr << "Failed to initialize PipeSpectrum" << std::endl;
        return 1;
    }

    // Run
    spectrumMeter.run();

    // Cleanup
    spectrumMeter.shutdown();
    g_spectrumMeter = nullptr;

    std::cout << "Goodbye!" << std::endl;
    return 0;
}
