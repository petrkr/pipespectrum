#include "Renderer.h"
#include <iostream>
#include <algorithm>

Renderer::Renderer(const WindowConfig& windowCfg, const VisualizationConfig& visCfg)
    : width(windowCfg.width), height(windowCfg.height),
      vsync(windowCfg.vsync), visConfig(visCfg) {
}

Renderer::~Renderer() {
    if (glContext) SDL_GL_DestroyContext(glContext);
    if (window) SDL_DestroyWindow(window);
    SDL_Quit();
}

bool Renderer::initialize() {
    // Prefer Wayland over X11 if available (SDL3 syntax)
    SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "wayland,x11");

    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cerr << "SDL initialization failed: " << SDL_GetError() << std::endl;
        return false;
    }

    // OpenGL attributes - use COMPATIBILITY profile for immediate mode (glBegin/glEnd)
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_COMPATIBILITY);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

    window = SDL_CreateWindow(
        "PipeSpectrum",
        width, height,
        SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE
    );

    if (!window) {
        std::cerr << "Window creation failed: " << SDL_GetError() << std::endl;
        return false;
    }

    glContext = SDL_GL_CreateContext(window);
    if (!glContext) {
        std::cerr << "OpenGL context creation failed: " << SDL_GetError() << std::endl;
        return false;
    }

    // Initialize GLEW
    glewExperimental = GL_TRUE;
    GLenum glewError = glewInit();
    if (glewError != GLEW_OK) {
        std::cerr << "GLEW initialization failed: " << glewGetErrorString(glewError) << std::endl;
        return false;
    }

    // VSync
    SDL_GL_SetSwapInterval(vsync ? 1 : 0);

    // OpenGL setup
    glViewport(0, 0, width, height);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    std::cout << "Renderer initialized: " << width << "x" << height << std::endl;
    std::cout << "OpenGL version: " << glGetString(GL_VERSION) << std::endl;

    return true;
}

void Renderer::clear() {
    glClear(GL_COLOR_BUFFER_BIT);
}

void Renderer::present() {
    SDL_GL_SwapWindow(window);
}

void Renderer::pollEvents() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_EVENT_QUIT) {
            closeRequested = true;
        } else if (event.type == SDL_EVENT_WINDOW_RESIZED) {
            width = event.window.data1;
            height = event.window.data2;
            glViewport(0, 0, width, height);
        } else if (event.type == SDL_EVENT_KEY_DOWN) {
            if (event.key.key == SDLK_ESCAPE || event.key.key == SDLK_Q) {
                closeRequested = true;
            }
        }
    }
}

void Renderer::renderSpectrum(const std::vector<float>& bands, const std::vector<float>& peaks, bool showPeaks) {
    if (bands.empty()) return;

    static int renderCount = 0;
    static float maxBandSeen = 0.0f;

    int numBands = static_cast<int>(bands.size());
    float totalGap = (numBands - 1) * visConfig.barGap;
    float barWidth = (width - totalGap) / static_cast<float>(numBands);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, width, 0, height, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    for (int i = 0; i < numBands; ++i) {
        maxBandSeen = std::max(maxBandSeen, bands[i]);
        float x = i * (barWidth + visConfig.barGap);
        float barHeight = bands[i] * height * 0.95f; // Leave 5% margin at top

        renderBar(x, 0, barWidth, barHeight, bands[i]);

        if (showPeaks && i < static_cast<int>(peaks.size())) {
            float peakY = peaks[i] * height * 0.95f;
            renderPeak(x, peakY, barWidth);
        }
    }

    if (++renderCount % 60 == 0) {
        std::cout << "[RENDER] Frame #" << renderCount << ", max band: " << maxBandSeen
                  << ", bars: " << numBands << ", width: " << barWidth << std::endl;
        maxBandSeen = 0.0f;
    }
}

void Renderer::renderBar(float x, float y, float width, float height, float level) {
    if (visConfig.barGradient) {
        // Gradient from bottom to top
        glBegin(GL_QUADS);

        // Bottom color (based on level)
        auto colorBottom = interpolateColor(level * 0.5f);
        glColor3f(colorBottom[0], colorBottom[1], colorBottom[2]);
        glVertex2f(x, y);
        glVertex2f(x + width, y);

        // Top color (based on level)
        auto colorTop = interpolateColor(level);
        glColor3f(colorTop[0], colorTop[1], colorTop[2]);
        glVertex2f(x + width, y + height);
        glVertex2f(x, y + height);

        glEnd();
    } else {
        auto color = interpolateColor(level);
        glColor3f(color[0], color[1], color[2]);

        glBegin(GL_QUADS);
        glVertex2f(x, y);
        glVertex2f(x + width, y);
        glVertex2f(x + width, y + height);
        glVertex2f(x, y + height);
        glEnd();
    }
}

void Renderer::renderPeak(float x, float y, float width) {
    glColor3f(
        visConfig.peakColor[0] / 255.0f,
        visConfig.peakColor[1] / 255.0f,
        visConfig.peakColor[2] / 255.0f
    );

    float peakHeight = 3.0f;

    glBegin(GL_QUADS);
    glVertex2f(x, y);
    glVertex2f(x + width, y);
    glVertex2f(x + width, y + peakHeight);
    glVertex2f(x, y + peakHeight);
    glEnd();
}

std::array<float, 3> Renderer::interpolateColor(float level) const {
    std::array<float, 3> color;

    if (level < 0.5f) {
        // Interpolate between low and mid
        float t = level * 2.0f;
        for (int i = 0; i < 3; ++i) {
            color[i] = (visConfig.barColorLow[i] * (1.0f - t) +
                       visConfig.barColorMid[i] * t) / 255.0f;
        }
    } else {
        // Interpolate between mid and high
        float t = (level - 0.5f) * 2.0f;
        for (int i = 0; i < 3; ++i) {
            color[i] = (visConfig.barColorMid[i] * (1.0f - t) +
                       visConfig.barColorHigh[i] * t) / 255.0f;
        }
    }

    return color;
}
