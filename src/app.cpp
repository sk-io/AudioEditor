#include "app.h"

#include "gui/mainwindow.h"
#include <QApplication>
#include <QDir>
#include <portaudio.h>

App the_app;

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
