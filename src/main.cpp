#include "SpectrumMeter.h"
#include "Config.h"
#include <iostream>
#include <signal.h>

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
    std::string configFile = "config.yaml";

    if (argc > 1) {
        configFile = argv[1];
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
