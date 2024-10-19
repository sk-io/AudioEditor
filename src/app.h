#ifndef APP_H
#define APP_H

#include "audiobuffer.h"
#include "audiointerface.h"

#include <QString>

class MainWindow;

struct App {
    AudioBuffer buffer;
    AudioBuffer clipboard;
    std::vector<AudioBuffer> history;
    MainWindow* main_window;
    QString file_path;
    QString last_dir;
    bool unsaved_changes;
    AudioInterface interface;
};

extern App the_app;

int run_app(int argc, char* argv[]);
void save_state();
void undo_state();

#endif // APP_H
