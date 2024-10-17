#include "app.h"

#include "gui/mainwindow.h"
#include <QApplication>
#include <QDir>
#include <portaudio.h>

App the_app;

struct {
    uint64_t pos = 0;
} playback;

static int test_audio_callback(const void *input_buf, void *output_buf,
               unsigned long num_frames,
               const PaStreamCallbackTimeInfo* time_info,
               PaStreamCallbackFlags status,
               void *user_data ) {


    const float* in = (const float*) the_app.buffer.get_raw_pointer();
    float* out = (float*) output_buf;

    unsigned long samples_left = the_app.buffer.get_num_samples() - (the_app.buffer.is_stereo() ? playback.pos / 2 : playback.pos);

    int num = std::min(num_frames, samples_left);
    if (the_app.buffer.is_stereo()) {
        for (uint64_t i = 0; i < num; i++) {
            *out++ = in[playback.pos++];
            *out++ = in[playback.pos++];
        }
    } else {
        for (uint64_t i = 0; i < num; i++) {
            *out++ = in[playback.pos++];
        }
    }

    return samples_left < num_frames ? paComplete : paContinue;
}

void test_play_audio() {
    playback.pos = 0;

    PaError err;

    err = Pa_Initialize();
    Q_ASSERT(err == paNoError);

    PaStreamParameters params;
    params.device = Pa_GetDefaultOutputDevice();
    Q_ASSERT(params.device != paNoDevice);

    params.channelCount = 2;
    params.sampleFormat = paFloat32;
    params.suggestedLatency = Pa_GetDeviceInfo(params.device)->defaultLowOutputLatency;
    params.hostApiSpecificStreamInfo = NULL;

    PaStream* stream;

    err = Pa_OpenStream(
        &stream,
        NULL,
        &params,
        44100,
        64,
        paClipOff,
        test_audio_callback,
        NULL
    );
    Q_ASSERT(err == paNoError);

    err = Pa_StartStream(stream);
    Q_ASSERT(err == paNoError);
}

int run_app(int argc, char* argv[]) {
    QApplication a(argc, argv);
    //the_app.buffer.load_from_file("piano1_short.ogg");
    the_app.last_dir = QDir::currentPath();
    the_app.buffer.init(2, 44100);
    the_app.unsaved_changes = false;

    MainWindow w;
    the_app.main_window = &w;
    w.show();
    return a.exec();
}

void save_state() {
    the_app.history.push_back(the_app.buffer);
}

void undo_state() {
    if (the_app.history.empty())
        return;

    the_app.buffer = the_app.history.back();
    the_app.history.pop_back();
}
