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

    float* out = (float*) output_buf;

    uint64_t frames_left = the_app.buffer.get_num_frames() - interface->m_frame_pos;

    uint64_t num = std::min((uint64_t) num_frames, frames_left);
    if (the_app.buffer.is_stereo()) {
        const float* in = (const float*) the_app.buffer.get_raw_pointer();
        in += interface->m_frame_pos * 2;
        for (uint64_t i = 0; i < num; i++) {
            *out++ = *in++;
            *out++ = *in++;
        }
    } else {
        const float* in = (const float*) the_app.buffer.get_raw_pointer();
        in += interface->m_frame_pos;
        for (uint64_t i = 0; i < num; i++) {
            *out++ = *in++;
        }
    }

    the_app.main_window->m_audio_widget->update();

    interface->m_frame_pos += num;
    return frames_left <= num_frames ? paComplete : paContinue;
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

    int num = Pa_GetHostApiCount();

    // for (int i = 0; i < num; i++) {
    //     const PaHostApiInfo* info = Pa_GetHostApiInfo(i);
    //     qDebug() << "------------------------------";
    //     qDebug() << info->name;

    //     for (int d = 0; d < info->deviceCount; d++) {
    //         int device_index = Pa_HostApiDeviceIndexToDeviceIndex(i, d);

    //         const PaDeviceInfo* info = Pa_GetDeviceInfo(device_index);

    //         qDebug() << "  " << info->name;
    //         qDebug() << "    " << info->defaultSampleRate << ", " << info->maxInputChannels << ", " << info->maxOutputChannels;
    //     }
    // }
}

void AudioInterface::set_pos(uint64_t frame_pos) {
    if (m_state != State::IDLE)
        return;

    m_frame_pos = frame_pos;
}

void AudioInterface::play() {
    if (m_state != State::IDLE)
        return;

    if (m_frame_pos >= the_app.buffer.get_num_frames())
        return;

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
