#include "app.h"

#include "gui/mainwindow.h"
#include <QApplication>
#include <QDir>
#include <portaudio.h>

App the_app;

int run_app(int argc, char* argv[]) {
    QApplication app(argc, argv);

    if (app.arguments().length() >= 2 && QFile::exists(app.arguments().at(1))) {
        the_app.buffer.load_from_file(app.arguments().at(1));
    } else {
        the_app.buffer.init(2, 44100);
    }

    the_app.last_dir = QDir::currentPath();
    the_app.unsaved_changes = false;

    the_app.interface.init();

    MainWindow window;
    the_app.main_window = &window;
    window.show();
    return app.exec();
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
