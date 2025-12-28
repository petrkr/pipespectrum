#include "AudioCapture.h"
#include <iostream>
#include <cstring>

AudioCapture::AudioCapture(int sampleRate, int bufferSize)
    : sampleRate(sampleRate), bufferSize(bufferSize) {
}

AudioCapture::~AudioCapture() {
    stop();
    if (stream) pw_stream_destroy(stream);
    if (context) pw_context_destroy(context);
    if (loop) pw_thread_loop_destroy(loop);
}

// Anonymous namespace for callbacks
namespace {
    void on_process_stream(void* userData) {
        AudioCapture::onProcessStream(userData);
    }

    void on_state_changed(void* userData, enum pw_stream_state old,
                          enum pw_stream_state state, const char* error) {
        AudioCapture::onStateChanged(userData, old, state, error);
    }
}

bool AudioCapture::initialize() {
    pw_init(nullptr, nullptr);

    // Create thread loop
    loop = pw_thread_loop_new("pipespectrum-loop", nullptr);
    if (!loop) {
        std::cerr << "Failed to create PipeWire thread loop" << std::endl;
        return false;
    }

    context = pw_context_new(pw_thread_loop_get_loop(loop), nullptr, 0);
    if (!context) {
        std::cerr << "Failed to create PipeWire context" << std::endl;
        return false;
    }

    // Create stream properties - CAPTURE_SINK captures playback audio (monitor)
    auto props = pw_properties_new(
        PW_KEY_MEDIA_TYPE, "Audio",
        PW_KEY_MEDIA_CATEGORY, "Capture",
        PW_KEY_MEDIA_ROLE, "Music",
        PW_KEY_STREAM_CAPTURE_SINK, "true",  // CRITICAL: Capture from sink (playback) not source (mic)
        nullptr
    );

    // Stream events
    static const struct pw_stream_events streamEvents = {
        .version = PW_VERSION_STREAM_EVENTS,
        .state_changed = on_state_changed,
        .process = on_process_stream,
    };

    // Create stream with simple API
    stream = pw_stream_new_simple(
        pw_thread_loop_get_loop(loop),
        "pipespectrum-capture",
        props,
        &streamEvents,
        this
    );

    if (!stream) {
        std::cerr << "Failed to create PipeWire stream" << std::endl;
        return false;
    }

    // Audio format parameters
    uint8_t buffer[1024];
    spa_pod_builder builder;
    spa_pod_builder_init(&builder, buffer, sizeof(buffer));

    struct spa_audio_info_raw audioInfo = {};
    audioInfo.format = SPA_AUDIO_FORMAT_F32;
    audioInfo.channels = 2;
    audioInfo.rate = static_cast<uint32_t>(sampleRate);

    const spa_pod* params[1];
    params[0] = (spa_pod*)spa_format_audio_raw_build(&builder, SPA_PARAM_EnumFormat, &audioInfo);

    // Connect stream to default sink monitor
    if (pw_stream_connect(stream,
                         PW_DIRECTION_INPUT,
                         PW_ID_ANY,  // Auto-select default output device
                         static_cast<pw_stream_flags>(
                             PW_STREAM_FLAG_AUTOCONNECT |
                             PW_STREAM_FLAG_MAP_BUFFERS),
                         params, 1) < 0) {
        std::cerr << "Failed to connect PipeWire stream" << std::endl;
        return false;
    }

    return true;
}

void AudioCapture::start() {
    if (running.load()) return;

    pw_thread_loop_start(loop);
    running.store(true);
    std::cout << "Audio capture started" << std::endl;
}

void AudioCapture::stop() {
    if (!running.load()) return;

    running.store(false);
    if (loop) pw_thread_loop_stop(loop);
    std::cout << "Audio capture stopped" << std::endl;
}

void AudioCapture::setCallback(AudioCallback cb) {
    std::lock_guard<std::mutex> lock(callbackMutex);
    callback = std::move(cb);
}

void AudioCapture::onProcessStream(void* userData) {
    auto* capture = static_cast<AudioCapture*>(userData);

    pw_buffer* buffer = pw_stream_dequeue_buffer(capture->stream);
    if (!buffer) {
        return;
    }

    spa_buffer* spaBuffer = buffer->buffer;

    // Validate buffer data
    if (!spaBuffer->datas[0].data || spaBuffer->datas[0].chunk->size == 0) {
        pw_stream_queue_buffer(capture->stream, buffer);
        return;
    }

    auto* samples = static_cast<float*>(spaBuffer->datas[0].data);
    uint32_t numSamples = spaBuffer->datas[0].chunk->size / sizeof(float);

    static int processCount = 0;
    static bool firstLog = true;
    if (firstLog || ++processCount % 100 == 0) {
        std::cout << "[CALLBACK] Processing audio: " << numSamples << " samples, count: "
                  << processCount << std::endl;
        firstLog = false;
    }

    capture->processAudio(samples, numSamples);

    pw_stream_queue_buffer(capture->stream, buffer);
}

void AudioCapture::onStateChanged(void* userData, enum pw_stream_state,
                                  enum pw_stream_state state, const char* error) {
    auto* capture = static_cast<AudioCapture*>(userData);

    std::cout << "Stream state changed: ";
    switch (state) {
        case PW_STREAM_STATE_ERROR:
            std::cout << "ERROR";
            if (error) std::cout << " - " << error;
            capture->running.store(false);
            break;
        case PW_STREAM_STATE_UNCONNECTED:
            std::cout << "UNCONNECTED";
            break;
        case PW_STREAM_STATE_CONNECTING:
            std::cout << "CONNECTING";
            break;
        case PW_STREAM_STATE_PAUSED:
            std::cout << "PAUSED";
            break;
        case PW_STREAM_STATE_STREAMING:
            std::cout << "STREAMING";
            break;
    }
    std::cout << std::endl;
}

void AudioCapture::processAudio(const float* samples, size_t count) {
    std::lock_guard<std::mutex> lock(callbackMutex);
    if (callback) {
        callback(samples, count);
    }
}
