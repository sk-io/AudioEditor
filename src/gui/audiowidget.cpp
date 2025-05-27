#include "audiowidget.h"
#include "../app.h"
#include "mainwindow.h"

#include <QPainter>
#include <QEvent>
#include <QMouseEvent>
#include <algorithm>

AudioWidget::AudioWidget(QWidget* parent) : QWidget{parent} {
    setMouseTracking(true);
    setAutoFillBackground(true);
}

void AudioWidget::select(double start, double end) {
    m_selection_pos_a = start;
    m_selection_pos_b = end;
    m_selection_state = start == end ? AudioWidget::SelectionState::MARKER : AudioWidget::SelectionState::REGION;
    m_state = State::IDLE;
}

void AudioWidget::deselect() {
    m_selection_state = SelectionState::DESELECTED;
    m_state = State::IDLE;
    setCursor(Qt::ArrowCursor);
}

void AudioWidget::set_view_mode(ViewMode mode) {
    m_view = mode;
    update();
}

double AudioWidget::get_selection_start_time() const {
    return std::min(m_selection_pos_a, m_selection_pos_b);
}

double AudioWidget::get_selection_end_time() const {
    return std::max(m_selection_pos_a, m_selection_pos_b);
}

void AudioWidget::paintEvent(QPaintEvent* event) {
    switch (m_view) {
    case ViewMode::OVERLAPPED:
        draw_single_view();
        break;
    case ViewMode::SPLIT_CHANNEL:
        draw_split_view();
        break;
    default:
        Q_ASSERT(false);
    }
}

void AudioWidget::draw_waveform_channel(int channel, QPainter& painter, int x0, int x1, int y0, int y1, const QColor& color) {
    for (int x = x0; x < x1; x++) {
        double time = x / m_pixels_per_second + m_scroll_pos;
        if (time < 0 || time >= the_app.buffer.get_duration())
            continue;

        int64_t start_frame = the_app.buffer.get_frame(time);
        int64_t end_frame = start_frame + the_app.buffer.get_frame(1.0 / m_pixels_per_second);

        float max, min;
        the_app.buffer.sample_amplitude(channel, start_frame, end_frame, max, min);

        painter.setPen(color);
        painter.drawLine(x, project_y(max, y0, y1), x, project_y(min, y0, y1));
    }
}

void AudioWidget::draw_single_view() {
    QPainter painter(this);
    painter.fillRect(rect(), Qt::black);

    QRect view_rect(rect());
    view_rect.setTop(m_timeline_height);

    // region selection
    if (m_selection_state == SelectionState::REGION) {
        int x0 = project_x(get_selection_start_time());
        int x1 = project_x(get_selection_end_time());

        int w = x1 - x0 + 1;

        painter.fillRect(x0, view_rect.top(), w, view_rect.height(), Qt::darkBlue);
    }

    // draw start line
    {
        int x = project_x(0);
        painter.setPen(Qt::darkGray);
        painter.drawLine(view_rect.left() + x, view_rect.top(), view_rect.left() + x, view_rect.bottom());
    }

    // marker selection
    if (m_selection_state == SelectionState::MARKER) {
        int x = project_x(m_selection_pos_a);
        painter.setPen(Qt::darkRed);
        painter.drawLine(x, view_rect.top(), x, view_rect.bottom());
    }

    // center line
    {
        painter.setPen(Qt::lightGray);
        painter.drawLine(view_rect.left(), view_rect.center().y(), view_rect.right(), view_rect.center().y());
    }

    // draw playback line
    if (the_app.interface.m_state != AudioInterface::State::IDLE) {
        uint64_t pos = the_app.interface.m_frame_pos;
        int x = (int) ((the_app.buffer.get_time(pos) - m_scroll_pos) * m_pixels_per_second);
        painter.setPen(Qt::yellow);
        painter.drawLine(view_rect.left() + x, view_rect.top(), view_rect.left() + x, view_rect.bottom());
    }

    const AudioBuffer& buffer = the_app.buffer;

    draw_waveform_channel(0, painter, view_rect.left(), view_rect.right(), view_rect.top(), view_rect.bottom(), Qt::red);

    if (the_app.buffer.get_num_channels() == 2)
        draw_waveform_channel(1, painter, view_rect.left(), view_rect.right(), view_rect.top(), view_rect.bottom(), Qt::green);

    draw_timeline(0, m_timeline_height);
}

void AudioWidget::draw_split_view() {
    QPainter painter(this);
    painter.fillRect(rect(), Qt::black);

    QRect view_rect(rect());
    view_rect.setTop(m_timeline_height);

    // region selection
    if (m_selection_state == SelectionState::REGION) {
        int x0 = project_x(get_selection_start_time());
        int x1 = project_x(get_selection_end_time());

        int w = x1 - x0 + 1;

        painter.fillRect(x0, view_rect.top(), w, view_rect.height(), Qt::darkBlue);
    }

    // draw start line
    {
        int x = project_x(0);
        painter.setPen(Qt::darkGray);
        painter.drawLine(view_rect.left() + x, view_rect.top(), view_rect.left() + x, view_rect.bottom());
    }

    // marker selection
    if (m_selection_state == SelectionState::MARKER) {
        int x = project_x(m_selection_pos_a);
        painter.setPen(Qt::darkRed);
        painter.drawLine(x, view_rect.top(), x, view_rect.bottom());
    }

    // main center line
    {
        painter.setPen(Qt::lightGray);
        painter.drawLine(view_rect.left(), view_rect.center().y(), view_rect.right(), view_rect.center().y());
    }

    int channel_height = view_rect.height() / 2;

    // channel center lines
    {
        painter.setPen(Qt::lightGray);
        painter.drawLine(view_rect.left(), view_rect.center().y() - channel_height / 2, view_rect.right(), view_rect.center().y() - channel_height / 2);
        painter.drawLine(view_rect.left(), view_rect.center().y() + channel_height / 2, view_rect.right(), view_rect.center().y() + channel_height / 2);
    }

    // draw playback line
    if (the_app.interface.m_state != AudioInterface::State::IDLE) {
        uint64_t pos = the_app.interface.m_frame_pos;
        int x = (int) ((the_app.buffer.get_time(pos) - m_scroll_pos) * m_pixels_per_second);
        painter.setPen(Qt::yellow);
        painter.drawLine(view_rect.left() + x, view_rect.top(), view_rect.left() + x, view_rect.bottom());
    }

    draw_waveform_channel(0, painter, view_rect.left(), view_rect.right(), view_rect.top(), view_rect.center().y(), Qt::red);

    if (the_app.buffer.get_num_channels() == 2)
        draw_waveform_channel(1, painter, view_rect.left(), view_rect.right(), view_rect.center().y(), view_rect.bottom(), Qt::green);

    draw_timeline(0, m_timeline_height);
}

void AudioWidget::draw_timeline(int y0, int y1) {
    QPainter painter(this);

    painter.fillRect(0, y0, rect().width(), y1 - y0, Qt::darkBlue);

    const int max_tick_width = 30;

    int subdivision_count = (int) std::log2(max_tick_width / (float) m_pixels_per_second);

    painter.setPen(Qt::white);

    float mul = std::pow(2.0f, (float)subdivision_count);
    float tick_width = m_pixels_per_second * mul;

    int tick0 = m_scroll_pos * m_pixels_per_second / tick_width;
    int num_ticks = width() / tick_width + 2;

    for (int i = tick0; i < tick0 + num_ticks; i++) {
        if (i < 0)
            continue;

        int x = project_x(i * mul);

        if (i % 2 == 0)
            painter.drawText(QPoint(x - 2, 15), tr("%1").arg(i * mul));

        painter.drawLine(x, m_timeline_height - 3, x, m_timeline_height - 1);
    }
}

bool AudioWidget::event(QEvent* event) {
    if (event->type() == QEvent::MouseMove) {
        QMouseEvent* mouse = (QMouseEvent*) event;

        m_mouse_x = mouse->pos().x() - rect().left();
        m_mouse_pos = m_scroll_pos + m_mouse_x / m_pixels_per_second;
        m_mouse_pos = std::max(m_mouse_pos, 0.0);

        if (m_state == State::SCROLLING) {
            m_scroll_pos = m_drag_start_scroll_pos - (m_mouse_x - m_drag_start_mouse_x) / m_pixels_per_second;
            update();
        } else if (m_state == State::SELECTING) {
            m_selection_pos_b = m_mouse_pos;
            update();
        } else if (m_state == State::RESIZE_REGION) {
            if (m_resizing_a)
                m_selection_pos_a = m_mouse_pos;
            else
                m_selection_pos_b = m_mouse_pos;
            update();
        } else {
            if (m_selection_state == SelectionState::REGION) {
                int select_a = project_x(m_selection_pos_a);
                int select_b = project_x(m_selection_pos_b);

                const int GRAB_WIDTH = 3;

                m_state = State::IDLE;
                if (abs(select_a - m_mouse_x) < GRAB_WIDTH) {
                    m_state = State::RESIZE_REGION_HOVER;
                    m_resizing_a = true;
                } else if (abs(select_b - m_mouse_x) < GRAB_WIDTH) {
                    m_state = State::RESIZE_REGION_HOVER;
                    m_resizing_a = false;
                }
            }
        }

        setCursor(m_state == State::RESIZE_REGION || m_state == State::RESIZE_REGION_HOVER ? Qt::SizeHorCursor : Qt::ArrowCursor);

        the_app.main_window->update_status_bar();
        return true;
    }

    if (event->type() == QEvent::MouseButtonPress || event->type() == QEvent::MouseButtonRelease) {
        bool pressed = event->type() == QEvent::MouseButtonPress;
        QMouseEvent* mouse = (QMouseEvent*) event;

        if (mouse->button() == Qt::RightButton && pressed && m_state == State::IDLE) {
            m_state = State::SCROLLING;
            m_drag_start_scroll_pos = m_scroll_pos;
            m_drag_start_mouse_x = mouse->pos().x() - rect().left();
        } else if (mouse->button() == Qt::RightButton && !pressed && m_state == State::SCROLLING) {
            m_state = State::IDLE;
        } else if (mouse->button() == Qt::LeftButton && pressed && m_state == State::IDLE) {
            m_state = State::SELECTING;
            m_selection_state = SelectionState::REGION;
            m_selection_pos_a = m_mouse_pos;
            m_selection_pos_b = m_mouse_pos;
        } else if (mouse->button() == Qt::LeftButton && !pressed && m_state == State::SELECTING) {
            m_state = State::IDLE;

            int ax = project_x(m_selection_pos_a);
            int bx = project_x(m_selection_pos_b);

            if (abs(ax - bx) < 2) {
                m_selection_pos_b = m_selection_pos_a;
                m_selection_state = SelectionState::MARKER;
            } else {
                m_selection_state = SelectionState::REGION;
            }
        } else if (mouse->button() == Qt::LeftButton && pressed && m_state == State::RESIZE_REGION_HOVER) {
            m_state = State::RESIZE_REGION;
        } else if (mouse->button() == Qt::LeftButton && !pressed && m_state == State::RESIZE_REGION) {
            m_state = State::IDLE;
            //setCursor(Qt::ArrowCursor);
        }

        update();
        return true;
    }

    if (event->type() == QEvent::Wheel) {
        QWheelEvent* wheel = (QWheelEvent*) event;

        if (m_state != State::IDLE && m_state != State::RESIZE_REGION_HOVER)
            return true;

        double old_scroll_pos = m_scroll_pos;
        double old_scroll_right_pos = m_scroll_pos + rect().width() / m_pixels_per_second;

        double lerp_val = m_mouse_x / (double) rect().width();

        double new_pixels_per_sec = m_pixels_per_second;
        if (wheel->angleDelta().y() > 0) {
            new_pixels_per_sec *= 1.5;
        } else {
            new_pixels_per_sec /= 1.5;
        }

        double new_width_seconds = rect().width() / new_pixels_per_sec;
        double scroll_max = old_scroll_right_pos - new_width_seconds;

        m_scroll_pos = old_scroll_pos + (scroll_max - old_scroll_pos) * lerp_val;
        m_pixels_per_second = new_pixels_per_sec;

        double sample_size_in_pixels = (1.0 / (double) the_app.buffer.get_sample_rate()) * m_pixels_per_second;
        //qDebug() << sample_size_in_pixels;

        update();
        return true;
    }

    return QWidget::event(event);
}

int AudioWidget::project_x(double time) const {
    return (int) ((time - m_scroll_pos) * m_pixels_per_second);
}

int AudioWidget::project_y(float amplitude, int y0, int y1) const {
    return y0 + (int)((-amplitude + 1.0f) / 2.0f * (float)(y1 - y0));
}
