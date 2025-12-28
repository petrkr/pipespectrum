#pragma once

#include "Config.h"
#include <SDL3/SDL.h>
#include <GL/glew.h>
#include <vector>

class Renderer {
public:
    Renderer(const WindowConfig& windowCfg, const VisualizationConfig& visCfg);
    ~Renderer();

    bool initialize();
    void clear();
    void present();

    void renderSpectrum(const std::vector<float>& bands, const std::vector<float>& peaks, bool showPeaks);

    bool shouldClose() const { return closeRequested; }
    void pollEvents();

    int getWidth() const { return width; }
    int getHeight() const { return height; }

private:
    void renderBar(float x, float y, float width, float height, float level);
    void renderPeak(float x, float y, float width);
    std::array<float, 3> interpolateColor(float level) const;

    SDL_Window* window = nullptr;
    SDL_GLContext glContext = nullptr;

    int width;
    int height;
    bool vsync;

    VisualizationConfig visConfig;
    bool closeRequested = false;
};
