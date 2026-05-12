#pragma once

#include "audio_buffer.h"
#include "audio_interface.h"
#include "waveform_cache.h"
#include "file_io.h"

#include <QString>

class MainWindow;

struct App {
    AudioBuffer buffer;
    AudioBuffer clipboard;
	FileIO io;
    std::vector<AudioBuffer> history;
    MainWindow* main_window;
    QString file_path;
    QString last_dir;
    bool unsaved_changes;
    AudioInterface interface;
    WaveformVisual waveform;
};

extern App the_app;

int run_app(int argc, char* argv[]);
void save_state();
void undo_state();
void show_error_box(const QString& msg);
