#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "audiowidget.h"

#include <QMainWindow>
#include <QLabel>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

    void update_status_bar();

private slots:
    void on_actionNew_triggered();
    void on_actionOpen_triggered();
    void on_actionSave_triggered();
    void on_actionSave_as_triggered();
    void on_actionDelete_triggered();
    void on_actionCopy_triggered();
    void on_actionPaste_triggered();
    void on_actionCut_triggered();
    void on_actionUndo_triggered();
    void on_actionRedo_triggered();
    void on_actionTrim_triggered();
    void on_actionDeselect_triggered();
    void on_actionSelect_All_triggered();
    void on_actionPlay_triggered();
    void on_actionStop_triggered();
    void on_actionRecord_triggered();
    void on_actionViewSingle_triggered();
    void on_actionViewSplit_triggered();
    void on_actionViewSpectrogram_triggered();
    void on_actionNormalize_triggered();
    void on_actionLoop_toggled(bool checked);

private:
    enum class Action {
        DELETE,
        COPY,
        CUT,
        PASTE,
        TRIM,
        NORMALIZE,
    };

    void load_from_file(const QString& path);
    void update_title();
    void perform_action(Action action);
    void dragEnterEvent(QDragEnterEvent *e);
    void dropEvent(QDropEvent *e);

public:
    Ui::MainWindow* ui;
    QLabel* m_file_info;
    QLabel* m_mouse_info;
    AudioWidget* m_audio_widget;
};

#endif // MAINWINDOW_H
