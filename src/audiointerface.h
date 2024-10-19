#ifndef AUDIOINTERFACE_H
#define AUDIOINTERFACE_H

#include <stdint.h>
#include <portaudio.h>

class AudioInterface {
public:
    AudioInterface() {}

    void set_pos(uint64_t frame_pos);
    void play();
    void record();
    void stop();

    enum class State {
        IDLE,
        PLAYING,
        RECORDING,
    };

private:
    State m_state = State::IDLE;
    uint64_t m_frame_pos = 0;
    PaStream* m_stream = nullptr;

    friend int playback_callback(const void *input_buf, void *output_buf,
        unsigned long num_frames, const PaStreamCallbackTimeInfo* time_info,
        PaStreamCallbackFlags status, void *user_data);
    friend void stream_finished(void* user_data);
    friend class MainWindow;
};

#endif // AUDIOINTERFACE_H
