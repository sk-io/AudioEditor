#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "audiowidget.h"
#include "../app.h"

#include <QFileDialog>
#include <QComboBox>
#include <QEvent>
#include <QMimeData>
#include <QDragEnterEvent>
#include <QDropEvent>

MainWindow::MainWindow(QWidget* parent) :
    QMainWindow(parent), ui(new Ui::MainWindow) {
    ui->setupUi(this);

    setAcceptDrops(true);

    m_audio_widget = new AudioWidget();
    ui->verticalLayout->addWidget(m_audio_widget);
    m_file_info = new QLabel();
    m_mouse_info = new QLabel();
    ui->statusbar->addPermanentWidget(m_mouse_info);
    ui->statusbar->addPermanentWidget(m_file_info);
    ui->toolBar->addSeparator();

    // {
    //     // API combo box
    //     QComboBox* api_box = new QComboBox();
    //     for (int i = 0; i < the_app.interface.get_num_apis(); i++) {
    //         api_box->addItem(the_app.interface.get_api_name(i));
    //     }
    //     connect(api_box, &QComboBox::currentIndexChanged, this, [this](int index) {
    //         the_app.interface.set_api(index);
    //     });

    //     ui->toolBar->addWidget(api_box);
    // }

    {
        // input device box
        QComboBox* input_device_box = new QComboBox();
        input_device_box->setMaximumWidth(300);
        for (int i = 0; i < the_app.interface.get_num_devices(); i++) {
            input_device_box->addItem(the_app.interface.get_device_name(i));
        }
        input_device_box->setCurrentIndex(the_app.interface.m_input_dev);
        ui->toolBar->addWidget(input_device_box);
    }

    {
        // input device box
        QComboBox* output_device_box = new QComboBox();
        output_device_box->setMaximumWidth(300);
        for (int i = 0; i < the_app.interface.get_num_devices(); i++) {
            output_device_box->addItem(the_app.interface.get_device_name(i));
        }
        output_device_box->setCurrentIndex(the_app.interface.m_output_dev);
        ui->toolBar->addWidget(output_device_box);
    }

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
    update_title();
    m_audio_widget->update();
}

void MainWindow::on_actionOpen_triggered() {
    QString path = QFileDialog::getOpenFileName(this, tr("Open"), the_app.last_dir, tr("Audio Files (*.wav *.mp3 *.ogg)"));
    if (path == nullptr)
        return;

    load_from_file(path);
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

void MainWindow::on_actionUndo_triggered() {
    undo_state();

    update_title();
    m_audio_widget->update();
}

void MainWindow::on_actionRedo_triggered() {
    Q_ASSERT(false);
}

void MainWindow::on_actionTrim_triggered() {
    perform_action(Action::TRIM);
}

void MainWindow::on_actionDeselect_triggered() {
    m_audio_widget->deselect();
    m_audio_widget->update();
}

void MainWindow::on_actionSelect_All_triggered() {
    m_audio_widget->select(0, the_app.buffer.get_duration());
    m_audio_widget->update();
}

void MainWindow::on_actionPlay_triggered() {
    if (the_app.interface.m_state != AudioInterface::State::IDLE)
        return;

    uint64_t frame_pos = 0;

    if (m_audio_widget->m_selection_state != AudioWidget::SelectionState::DESELECTED)
        frame_pos = the_app.buffer.get_frame(m_audio_widget->m_selection_pos_a);

    the_app.interface.set_pos(frame_pos);
    the_app.interface.play();
}

void MainWindow::on_actionStop_triggered() {
    the_app.interface.stop();
    m_audio_widget->update();
}

void MainWindow::on_actionRecord_triggered() {

}

void MainWindow::load_from_file(const QString& path) {
    if (!the_app.buffer.load_from_file(path)) {
        return;
    }

    QFileInfo info(path);
    the_app.file_path = path;
    the_app.last_dir = info.dir().path();
    the_app.unsaved_changes = false;

    update_status_bar();
    update_title();
    m_audio_widget->deselect();
    m_audio_widget->update();
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

    int64_t start = the_app.buffer.get_frame(select_start);
    int64_t end = the_app.buffer.get_frame(select_end);

    switch (action) {
    case Action::DELETE:
        if (m_audio_widget->m_selection_state != AudioWidget::SelectionState::REGION)
            break;
        save_state();
        the_app.buffer.delete_region(start, end);
        m_audio_widget->deselect();
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
            m_audio_widget->deselect();
        }
        break;
    case Action::CUT:
        if (m_audio_widget->m_selection_state != AudioWidget::SelectionState::REGION)
            break;
        m_audio_widget->deselect();
        save_state();
        the_app.buffer.cut_region(start, end, the_app.clipboard);
        the_app.unsaved_changes = true;
        break;
    case Action::TRIM: {
        if (m_audio_widget->m_selection_state != AudioWidget::SelectionState::REGION)
            break;
        m_audio_widget->deselect();
        save_state();
        AudioBuffer temp;
        the_app.buffer.copy_region(start, end, temp);
        the_app.buffer = std::move(temp);
        the_app.unsaved_changes = true;
        break;
    }
    default:
        Q_ASSERT(false);
    }

    update_title();
    m_audio_widget->update();
}

void MainWindow::dragEnterEvent(QDragEnterEvent* event) {
    if (event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
    }
}

void MainWindow::dropEvent(QDropEvent* event) {
    if (event->mimeData()->urls().length() == 0)
        return;

    QString path = event->mimeData()->urls()[0].toLocalFile();
    load_from_file(path);
}
