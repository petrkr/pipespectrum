#pragma once

#include <pipewire/pipewire.h>
#include <spa/param/audio/format-utils.h>
#include <vector>
#include <functional>
#include <mutex>
#include <thread>
#include <atomic>

class AudioCapture {
public:
    using AudioCallback = std::function<void(const float* samples, size_t count)>;

    AudioCapture(int sampleRate, int bufferSize);
    ~AudioCapture();

    bool initialize();
    void start();
    void stop();

    void setCallback(AudioCallback callback);

    bool isRunning() const { return running.load(); }

    // Public for callbacks
    static void onProcessStream(void* userData);
    static void onStateChanged(void* userData, enum pw_stream_state old,
                               enum pw_stream_state state, const char* error);

private:

    void processAudio(const float* samples, size_t count);

    int sampleRate;
    int bufferSize;
    AudioCallback callback;

    pw_thread_loop* loop = nullptr;
    pw_stream* stream = nullptr;
    pw_context* context = nullptr;

    std::atomic<bool> running{false};
    std::mutex callbackMutex;
};
