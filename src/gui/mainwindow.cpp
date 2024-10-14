#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "audiowidget.h"
#include "../app.h"

#include <QFileDialog>

MainWindow::MainWindow(QWidget* parent) :
    QMainWindow(parent), ui(new Ui::MainWindow) {
    ui->setupUi(this);
    m_audio_widget = new AudioWidget();
    ui->verticalLayout->addWidget(m_audio_widget);
    m_file_info = new QLabel();
    m_mouse_info = new QLabel();
    ui->statusbar->addPermanentWidget(m_mouse_info);
    ui->statusbar->addPermanentWidget(m_file_info);
    update_status_bar();
    update_title();
}

MainWindow::~MainWindow() {
    delete ui;
}

void MainWindow::update_status_bar() {
    // TODO: reorganize
    QString str =
        QString("Length: %1s %2 %3Hz")
        .arg(the_app.buffer.get_duration())
        .arg(the_app.buffer.get_num_channels() == 2 ? "Stereo" : "Mono")
        .arg(the_app.buffer.get_sample_rate());
    m_file_info->setText(str);
    m_mouse_info->setText(QString("Time: %1s").arg(m_audio_widget->get_mouse_pos()));
}

void MainWindow::on_actionNew_triggered() {
    the_app.buffer.init(2, 44100);
    the_app.file_path = "";
    the_app.unsaved_changes = false;
    update_status_bar();
    m_audio_widget->repaint();
    update_title();
}

void MainWindow::on_actionOpen_triggered() {
    QString path = QFileDialog::getOpenFileName(this, tr("Open"), the_app.last_dir, tr("Audio Files (*.wav *.mp3 *.ogg)"));
    if (path == nullptr)
        return;

    QFileInfo info(path);
    the_app.buffer.load_from_file(path);
    the_app.file_path = path;
    the_app.last_dir = info.dir().path();
    the_app.unsaved_changes = false;
    update_status_bar();
    update_title();
}

void MainWindow::on_actionSave_triggered() {
    if (the_app.file_path == "") {
        on_actionSave_as_triggered();
        return;
    }

    the_app.buffer.save_to_file(the_app.file_path);
    the_app.unsaved_changes = false;
    update_title();
}

void MainWindow::on_actionSave_as_triggered() {
    QString path = QFileDialog::getSaveFileName(this, tr("Save as"), the_app.last_dir, tr("Audio Files (*.wav *.mp3 *.ogg)"));
    if (path == nullptr)
        return;

    QFileInfo info(path);
    the_app.file_path = path;
    the_app.last_dir = info.dir().path();
    the_app.buffer.save_to_file(the_app.file_path);
    the_app.unsaved_changes = false;
    update_title();
}

void MainWindow::on_actionDelete_triggered() {
    perform_action(Action::DELETE);
}

void MainWindow::on_actionCopy_triggered() {
    perform_action(Action::COPY);
}

void MainWindow::on_actionPaste_triggered() {
    perform_action(Action::PASTE);
}

void MainWindow::on_actionCut_triggered() {
    perform_action(Action::CUT);
}

void MainWindow::update_title() {
    QString file_name;
    if (the_app.file_path == "") {
        file_name = "Untitled";
    } else {
        QFileInfo file_info(the_app.file_path);
        file_name = file_info.fileName();
    }

    setWindowTitle(QString("%1%2 - AudioEditor").arg(the_app.unsaved_changes ? "*" : "", file_name));
}

void MainWindow::perform_action(Action action) {
    double select_start = std::min(m_audio_widget->m_selection_pos_a, m_audio_widget->m_selection_pos_b);
    double select_end = std::max(m_audio_widget->m_selection_pos_a, m_audio_widget->m_selection_pos_b);

    int64_t start = the_app.buffer.get_sample_index(select_start);
    int64_t end = the_app.buffer.get_sample_index(select_end);

    switch (action) {
    case Action::DELETE:
        if (m_audio_widget->m_selection_state != AudioWidget::SelectionState::REGION)
            break;
        save_state();
        the_app.buffer.delete_region(start, end);
        m_audio_widget->m_selection_state = AudioWidget::SelectionState::DESELECTED;
        the_app.unsaved_changes = true;
        break;
    case Action::COPY:
        if (m_audio_widget->m_selection_state != AudioWidget::SelectionState::REGION)
            break;
        the_app.buffer.copy_region(start, end, the_app.clipboard);
        break;
    case Action::PASTE:
        if (m_audio_widget->m_selection_state == AudioWidget::SelectionState::MARKER) {
            save_state();
            the_app.buffer.paste_from(start, the_app.clipboard);
            the_app.unsaved_changes = true;
        } else if (m_audio_widget->m_selection_state == AudioWidget::SelectionState::REGION) {
            save_state();
            the_app.buffer.delete_region(start, end);
            the_app.buffer.paste_from(start, the_app.clipboard);
            the_app.unsaved_changes = true;
            m_audio_widget->m_selection_state = AudioWidget::SelectionState::DESELECTED;
        }
        break;
    case Action::CUT:
        if (m_audio_widget->m_selection_state != AudioWidget::SelectionState::REGION)
            break;
        m_audio_widget->m_selection_state = AudioWidget::SelectionState::DESELECTED;
        save_state();
        the_app.buffer.cut_region(start, end, the_app.clipboard);
        the_app.unsaved_changes = true;
        break;
    default:
        Q_ASSERT(false);
    }

    update_title();
    m_audio_widget->repaint();
}

void MainWindow::on_actionUndo_triggered() {
    undo_state();

    update_title();
    m_audio_widget->repaint();
}

void MainWindow::on_actionRedo_triggered() {
    Q_ASSERT(false);
}
