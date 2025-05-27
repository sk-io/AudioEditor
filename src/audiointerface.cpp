#include "audiointerface.h"

#include "app.h"
#include <QtGlobal>
#include <qlogging.h>
#include <QDebug>
#include "gui/mainwindow.h"


static int playback_callback(const void* input_buf, void* output_buf,
                             unsigned long num_frames, const PaStreamCallbackTimeInfo* time_info,
                             PaStreamCallbackFlags status, void* user_data) {
    AudioInterface* interface = (AudioInterface*) user_data;

    if (interface->m_stop_pos != -1 && interface->m_frame_pos >= interface->m_stop_pos) {
        if (interface->m_loop)
            interface->m_frame_pos = interface->m_start_pos;
        else
            return paComplete;
    }

    float* out = (float*) output_buf;

    int64_t frames_left = interface->m_stop_pos - interface->m_frame_pos;

    int64_t num = std::min((int64_t) num_frames, frames_left);
    if (the_app.buffer.is_stereo()) {
        const float* in = (const float*) the_app.buffer.get_raw_pointer();
        in += interface->m_frame_pos * 2;
        for (int64_t i = 0; i < num; i++) {
            *out++ = *in++;
            *out++ = *in++;
        }
    } else {
        const float* in = (const float*) the_app.buffer.get_raw_pointer();
        in += interface->m_frame_pos;
        for (int64_t i = 0; i < num; i++) {
            *out++ = *in++;
        }
    }

    // TODO: this is not thread safe!
    the_app.main_window->m_audio_widget->update();

    interface->m_frame_pos += num;
    return paContinue;
}

static void stream_finished(void* user_data) {
    AudioInterface* interface = (AudioInterface*) user_data;

    interface->m_state = AudioInterface::State::IDLE;
    the_app.main_window->m_audio_widget->update();
}

void AudioInterface::init() {
    PaError err;

    err = Pa_Initialize();
    Q_ASSERT(err == paNoError);

    m_input_dev = Pa_GetDefaultInputDevice();
    Q_ASSERT(m_input_dev != paNoDevice);
    m_output_dev = Pa_GetDefaultOutputDevice();
    Q_ASSERT(m_output_dev != paNoDevice);
}

void AudioInterface::play(int64_t start_pos, int64_t stop_pos) {
    if (m_state != State::IDLE)
        return;

    m_start_pos = std::min(start_pos, the_app.buffer.get_num_frames());
    m_stop_pos = std::min(stop_pos, the_app.buffer.get_num_frames());
    m_frame_pos = m_start_pos;

    PaError err;

    PaStreamParameters params;
    params.device = m_output_dev;

    params.channelCount = the_app.buffer.get_num_channels();
    params.sampleFormat = paFloat32;
    params.suggestedLatency = Pa_GetDeviceInfo(params.device)->defaultLowOutputLatency;
    params.hostApiSpecificStreamInfo = NULL;

    err = Pa_OpenStream(
        &m_stream,
        NULL,
        &params,
        the_app.buffer.get_sample_rate(),
        64,
        paClipOff,
        playback_callback,
        this
    );
    Q_ASSERT(err == paNoError);

    err = Pa_SetStreamFinishedCallback(m_stream, stream_finished);
    Q_ASSERT(err == paNoError);

    err = Pa_StartStream(m_stream);
    Q_ASSERT(err == paNoError);

    m_state = State::PLAYING;
}

void AudioInterface::record() {
}

void AudioInterface::stop() {
    if (m_state == State::IDLE)
        return;

    if (Pa_IsStreamStopped(m_stream))
        return;

    PaError err;
    err = Pa_StopStream(m_stream);
    Q_ASSERT(err == paNoError);
    //m_state = State::IDLE;
}

int AudioInterface::get_num_apis() const {
    return Pa_GetHostApiCount();
}

void AudioInterface::set_api(int i) {
    m_api = i;
}

QString AudioInterface::get_api_name(int i) {
    return Pa_GetHostApiInfo(i)->name;
}

int AudioInterface::get_num_devices() const {
    return Pa_GetDeviceCount();
}

QString AudioInterface::get_device_name(int i) {
    const auto* device_info = Pa_GetDeviceInfo(i);
    const auto* api_info = Pa_GetHostApiInfo(device_info->hostApi);

    return QString(api_info->name) + ": " + QString(device_info->name);
}

void AudioInterface::set_input_device(int i) {
    m_input_dev = i;
}

void AudioInterface::set_output_device(int i) {
    m_output_dev = i;
}
