#include <jni.h>
#include <string>
#include <oboe/Oboe.h>
#include <android/log.h>

#define TAG "TWSNoiseCancellerLib"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__)

class NoiseCancellerEngine : public oboe::AudioStreamDataCallback {
public:
    NoiseCancellerEngine() : recordingStream(nullptr), playbackStream(nullptr), isPlaying(false) {}

    ~NoiseCancellerEngine() {
        stop();
    }

    bool start() {
        if (isPlaying) return true;

        // Note: For real-time processing between mic and speaker,
        // we set up the Output stream first, and the Input stream sends data to the Output stream.
        // However, because we need to perform processing (DSP), we'll read from Input
        // and write to Output inside the AudioStreamDataCallback.
        // The callback is attached to the Output Stream.

        oboe::AudioStreamBuilder outBuilder;
        outBuilder.setDirection(oboe::Direction::Output)
                  ->setPerformanceMode(oboe::PerformanceMode::LowLatency)
                  ->setSharingMode(oboe::SharingMode::Exclusive)
                  ->setFormat(oboe::AudioFormat::Float)
                  ->setChannelCount(oboe::ChannelCount::Mono)
                  ->setDataCallback(this)
                  ->setErrorCallback(nullptr); // Simplified for this example

        oboe::Result result = outBuilder.openStream(playbackStream);
        if (result != oboe::Result::OK) {
            LOGE("Failed to open playback stream: %s", oboe::convertToText(result));
            return false;
        }

        // Use the output stream's properties for the input stream
        oboe::AudioStreamBuilder inBuilder;
        inBuilder.setDirection(oboe::Direction::Input)
                 ->setPerformanceMode(oboe::PerformanceMode::LowLatency)
                 ->setSharingMode(oboe::SharingMode::Exclusive)
                 ->setFormat(oboe::AudioFormat::Float)
                 ->setSampleRate(playbackStream->getSampleRate())
                 ->setChannelCount(oboe::ChannelCount::Mono)
                 ->setInputPreset(oboe::InputPreset::VoiceCommunication); // Good for SCO

        result = inBuilder.openStream(recordingStream);
        if (result != oboe::Result::OK) {
            LOGE("Failed to open recording stream: %s", oboe::convertToText(result));
            playbackStream->close();
            return false;
        }

        // Start streams
        result = recordingStream->requestStart();
        if (result != oboe::Result::OK) {
            LOGE("Failed to start recording stream: %s", oboe::convertToText(result));
            return false;
        }

        result = playbackStream->requestStart();
        if (result != oboe::Result::OK) {
            LOGE("Failed to start playback stream: %s", oboe::convertToText(result));
            return false;
        }

        isPlaying = true;
        LOGD("Noise Canceller Started");
        return true;
    }

    void stop() {
        if (!isPlaying) return;

        if (playbackStream) {
            playbackStream->requestStop();
            playbackStream->close();
            playbackStream.reset();
        }

        if (recordingStream) {
            recordingStream->requestStop();
            recordingStream->close();
            recordingStream.reset();
        }

        isPlaying = false;
        LOGD("Noise Canceller Stopped");
    }

    // This callback is called when the Playback stream needs data
    oboe::DataCallbackResult onAudioReady(oboe::AudioStream *oboeStream, void *audioData, int32_t numFrames) override {
        float *outputBuffer = static_cast<float *>(audioData);

        if (recordingStream) {
            // Read from the microphone buffer
            float inputBuffer[numFrames];
            
            // Note: In a robust app, handle timeouts and partial reads
            auto result = recordingStream->read(inputBuffer, numFrames, 0); 
            
            if (result.value() == numFrames) {
                
                // =======================================================
                // DSP (Digital Signal Processing) starts here
                // =======================================================
                // This is where RNNoise or a custom DSP noise cancellation block goes.
                // It processes the float array `inputBuffer` and outputs to `outputBuffer`.
                
                for (int i = 0; i < numFrames; i++) {
                    // Placeholder constraint: Output modified float buffer
                    // e.g. outputBuffer[i] = rnnoise_process_frame(state, inputBuffer[i]);
                    // For now, pass-through with slight attenuation
                    outputBuffer[i] = inputBuffer[i] * 0.9f; 
                }
                
                // =======================================================
                // DSP ends here
                // =======================================================

            } else {
                // If we couldn't read enough frames, output silence
                memset(outputBuffer, 0, numFrames * sizeof(float));
            }
        } else {
            memset(outputBuffer, 0, numFrames * sizeof(float));
        }

        return oboe::DataCallbackResult::Continue;
    }

private:
    std::shared_ptr<oboe::AudioStream> recordingStream;
    std::shared_ptr<oboe::AudioStream> playbackStream;
    bool isPlaying;
};

// Global instance of the engine
NoiseCancellerEngine* engine = nullptr;

extern "C"
JNIEXPORT jboolean JNICALL
Java_com_example_twsnoisecanceller_MainActivity_startDenoise(JNIEnv *env, jobject thiz) {
    if (engine == nullptr) {
        engine = new NoiseCancellerEngine();
    }
    return engine->start() ? JNI_TRUE : JNI_FALSE;
}

extern "C"
JNIEXPORT void JNICALL
Java_com_example_twsnoisecanceller_MainActivity_stopDenoise(JNIEnv *env, jobject thiz) {
    if (engine != nullptr) {
        engine->stop();
        delete engine;
        engine = nullptr;
    }
}
