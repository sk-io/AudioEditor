#ifndef AUDIOINTERFACE_H
#define AUDIOINTERFACE_H

#include <stdint.h>
#include <portaudio.h>
#include <QString>

class AudioInterface {
public:
    AudioInterface() {}

    void init();
    void set_pos(uint64_t frame_pos);
    void play();
    void record();
    void stop();

    int get_num_apis() const;
    void set_api(int i);
    QString get_api_name(int i);

    int get_num_devices() const;
    QString get_device_name(int i);

    void set_input_device(int i);
    void set_output_device(int i);

    enum class State {
        IDLE,
        PLAYING,
        RECORDING,
    };

private:
    State m_state = State::IDLE;
    uint64_t m_frame_pos = 0;
    PaStream* m_stream = nullptr;
    int m_api = -1;
    int m_input_dev = -1, m_output_dev = -1;

    friend int playback_callback(const void *input_buf, void *output_buf,
        unsigned long num_frames, const PaStreamCallbackTimeInfo* time_info,
        PaStreamCallbackFlags status, void *user_data);
    friend void stream_finished(void* user_data);
    friend class MainWindow;
    friend class AudioWidget;
};

#endif // AUDIOINTERFACE_H
