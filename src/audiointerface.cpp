#include "audiointerface.h"

#include "app.h"
#include <QtAssert>
#include <QtLogging>
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
    //qDebug() << "stream finished!";
    interface->m_state = AudioInterface::State::IDLE;
    the_app.main_window->m_audio_widget->update();
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

    err = Pa_Initialize();
    Q_ASSERT(err == paNoError);

    PaStreamParameters params;
    params.device = Pa_GetDefaultOutputDevice();
    Q_ASSERT(params.device != paNoDevice);

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

    PaError err;
    err = Pa_StopStream(m_stream);
    Q_ASSERT(err == paNoError);
    //m_state = State::IDLE;
}
