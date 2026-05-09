#include "audiowidget.h"
#include "../app.h"
#include "mainwindow.h"

#include <QPainter>
#include <QEvent>
#include <QMouseEvent>
#include <algorithm>

// how many pixels wide does a sample have to be to switch to graph mode?
const double graph_pixel_threshold = 0.5;

AudioWidget::AudioWidget(QWidget* parent) : QWidget{parent} {
    setMouseTracking(true);
    setAutoFillBackground(true);
    m_pixels_per_second = pow(1.5, m_zoom);
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

void AudioWidget::draw_waveform_stereo(QPainter& painter, int x0, int x1, int y0, int y1) {
	const AudioBuffer& buffer = the_app.buffer;
    double frames_per_pixel = buffer.get_sample_rate() / m_pixels_per_second;
    double pixels_per_frame = m_pixels_per_second / buffer.get_sample_rate();
	//qDebug() << frames_per_pixel;

    const WaveformVisual& waveform = the_app.waveform;
	const QColor& left_color = Qt::red;
	const QColor& right_color = Qt::green;
	//const QColor& combined_color = Qt::white;
	const QColor& combined_color = QColor(180, 255, 120);

//	const QColor left_color("#00D1FF");
//	const QColor right_color("#FF2DA6");
//	const QColor combined_color("#FFFFFF");
	if (pixels_per_frame >= graph_pixel_threshold) {
		draw_waveform_graph(0, painter, x0, x1, y0, y1, left_color);
		draw_waveform_graph(1, painter, x0, x1, y0, y1, right_color);
		return;
	}

    int level_i = waveform.find_best_level(frames_per_pixel);

    for (int x = x0; x < x1; x++) {
        double time = x / m_pixels_per_second + m_scroll_pos;
        if (time < 0 || time >= the_app.buffer.get_duration())
            continue;

        int64_t start_frame = the_app.buffer.get_frame(time);
        int64_t end_frame = start_frame + the_app.buffer.get_frame(1.0 / m_pixels_per_second);

		float left_min, left_max;
		float right_min, right_max;

		if (level_i >= 0) {
			waveform.sample(start_frame, end_frame, level_i, 0, left_min, left_max);
			waveform.sample(start_frame, end_frame, level_i, 1, right_min, right_max);
		} else {
			the_app.buffer.sample_amplitude(0, start_frame, end_frame, left_max, left_min);
			the_app.buffer.sample_amplitude(1, start_frame, end_frame, right_max, right_min);
		}

		int left_y0 = project_y(left_max, y0, y1);
		int left_y1 = project_y(left_min, y0, y1);
		
		int right_y0 = project_y(right_max, y0, y1);
		int right_y1 = project_y(right_min, y0, y1);

        painter.setPen(left_color);
        painter.drawLine(x, left_y0, x, left_y1);
        painter.setPen(right_color);
        painter.drawLine(x, right_y0, x, right_y1);

		if (left_y1 < right_y0)
			continue;
		if (right_y0 > left_y1)
			continue;
		if (frames_per_pixel <= 10.0)
			continue;

		painter.setPen(combined_color);
		painter.drawLine(x, std::max(left_y0, right_y0), x, std::min(left_y1, right_y1));
    }
}

void AudioWidget::draw_waveform_mono(int channel, QPainter& painter, int x0, int x1, int y0, int y1, const QColor& color) {
	const AudioBuffer& buffer = the_app.buffer;
    double frames_per_pixel = buffer.get_sample_rate() / m_pixels_per_second;
    double pixels_per_frame = m_pixels_per_second / buffer.get_sample_rate();

    const WaveformVisual& waveform = the_app.waveform;

    int level_i = waveform.find_best_level(frames_per_pixel);

	if (pixels_per_frame >= graph_pixel_threshold) {
		draw_waveform_graph(channel, painter, x0, x1, y0, y1, color);
		return;
	}

    for (int x = x0; x < x1; x++) {
        double time = x / m_pixels_per_second + m_scroll_pos;
        if (time < 0 || time >= the_app.buffer.get_duration())
            continue;

        int64_t start_frame = the_app.buffer.get_frame(time);
        int64_t end_frame = start_frame + the_app.buffer.get_frame(1.0 / m_pixels_per_second);

        float max, min;
        if (level_i >= 0)
            waveform.sample(start_frame, end_frame, level_i, channel, min, max);
        else
            the_app.buffer.sample_amplitude(channel, start_frame, end_frame, max, min);

        painter.setPen(color);
        painter.drawLine(x, project_y(max, y0, y1), x, project_y(min, y0, y1));
    }
}

void AudioWidget::draw_waveform_graph(int channel, QPainter& painter, int x0, int x1, int y0, int y1, const QColor& color) {
	const AudioBuffer& buffer = the_app.buffer;
    double pixels_per_frame = m_pixels_per_second / buffer.get_sample_rate();

	int64_t x0_frame = buffer.clamp_frame(buffer.get_frame(m_scroll_pos));
	double x1_time = (x1 - x0) / m_pixels_per_second + m_scroll_pos;
	int64_t x1_frame = buffer.clamp_frame(buffer.get_frame(x1_time) + 2);

	const int grabber_size = 10;

	painter.setPen(color);
	int prev_x = -1, prev_y = -1;
	for (int64_t f = x0_frame; f < x1_frame; f++) {
		double time = buffer.get_time(f);
        int x = (time - m_scroll_pos) * m_pixels_per_second;
		int y = project_y(buffer.single_sample(f, channel), y0, y1);
		
		if (prev_y != -1)
			painter.drawLine(prev_x, prev_y, x, y);

		if (pixels_per_frame > grabber_size * 1.5)
			painter.drawRect(x - grabber_size / 2, y - grabber_size / 2, grabber_size, grabber_size);

		prev_x = x;
		prev_y = y;
	}
}

void AudioWidget::draw_single_view() {
    QPainter painter(this);
    painter.fillRect(rect(), Qt::black);

    QRect view_rect(rect());
    view_rect.setTop(m_timeline_height);


    // draw start/end lines
    {
        int start_x = view_rect.left() + project_x(0);
        int end_x   = view_rect.left() + project_x(the_app.buffer.get_duration());
        painter.setPen(Qt::darkGray);
        painter.drawLine(start_x, view_rect.top(), start_x, view_rect.bottom());
        painter.drawLine(end_x, view_rect.top(), end_x, view_rect.bottom());
    }

    // marker selection
    if (m_selection_state == SelectionState::MARKER) {
        int x = project_x(m_selection_pos_a);
        painter.setPen(Qt::darkRed);
        painter.drawLine(x, view_rect.top(), x, view_rect.bottom());
    }


    // draw playback line
    if (the_app.interface.m_state != AudioInterface::State::IDLE) {
        uint64_t pos = the_app.interface.m_frame_pos;
        int x = (int) ((the_app.buffer.get_time(pos) - m_scroll_pos) * m_pixels_per_second);
        painter.setPen(Qt::yellow);
        painter.drawLine(view_rect.left() + x, view_rect.top(), view_rect.left() + x, view_rect.bottom());
    }

    const AudioBuffer& buffer = the_app.buffer;
	bool stereo = buffer.get_num_channels() == 2;

	painter.setPen(Qt::white);
	painter.drawText(5, 38, stereo ? "Stereo" : "Mono");

	// draw the actual waveform
    if (stereo) {
        draw_waveform_stereo(painter, view_rect.left(), view_rect.right(), view_rect.top(), view_rect.bottom());
	} else {
		draw_waveform_mono(0, painter, view_rect.left(), view_rect.right(), view_rect.top(), view_rect.bottom(), Qt::red);
	}

    // center line
    {
        painter.setPen(QColor(0, 0, 0, 30));
        painter.drawLine(view_rect.left(), view_rect.center().y(), view_rect.right(), view_rect.center().y());
    }

    // region selection
    if (m_selection_state == SelectionState::REGION) {
		const QColor color(100, 100, 255, 120);
        int x0 = project_x(get_selection_start_time());
        int x1 = project_x(get_selection_end_time());

        int w = x1 - x0 + 1;

        painter.fillRect(x0, view_rect.top(), w, view_rect.height(), color);
    }

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

    // draw start/end lines
    {
        int start_x = view_rect.left() + project_x(0);
        int end_x   = view_rect.left() + project_x(the_app.buffer.get_duration());
        painter.setPen(Qt::darkGray);
        painter.drawLine(start_x, view_rect.top(), start_x, view_rect.bottom());
        painter.drawLine(end_x, view_rect.top(), end_x, view_rect.bottom());
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


    // draw playback line
    if (the_app.interface.m_state != AudioInterface::State::IDLE) {
        uint64_t pos = the_app.interface.m_frame_pos;
        int x = (int) ((the_app.buffer.get_time(pos) - m_scroll_pos) * m_pixels_per_second);
        painter.setPen(Qt::yellow);
        painter.drawLine(view_rect.left() + x, view_rect.top(), view_rect.left() + x, view_rect.bottom());
    }

    draw_waveform_mono(0, painter, view_rect.left(), view_rect.right(), view_rect.top(), view_rect.center().y(), Qt::red);

    if (the_app.buffer.get_num_channels() == 2)
        draw_waveform_mono(1, painter, view_rect.left(), view_rect.right(), view_rect.center().y(), view_rect.bottom(), Qt::green);

	painter.setPen(Qt::white);
	painter.drawText(5, 38, "Left");
	painter.drawText(5, view_rect.center().y() + 16, "Right");

    // channel center lines
    {
        painter.setPen(QColor(0, 0, 0, 30));
        painter.drawLine(view_rect.left(), view_rect.center().y() - channel_height / 2, view_rect.right(), view_rect.center().y() - channel_height / 2);
        painter.drawLine(view_rect.left(), view_rect.center().y() + channel_height / 2, view_rect.right(), view_rect.center().y() + channel_height / 2);
    }

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
        m_mouse_pos = std::min(m_mouse_pos, the_app.buffer.get_duration());

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

        if (wheel->angleDelta().y() != 0) {
            if (m_state != State::IDLE && m_state != State::RESIZE_REGION_HOVER)
                return true;

            double old_scroll_pos = m_scroll_pos;
            double old_scroll_right_pos = m_scroll_pos + rect().width() / m_pixels_per_second;

            double lerp_val = m_mouse_x / (double) rect().width();

            set_zoom(m_zoom + wheel->angleDelta().y() * 0.01);

            double new_width_seconds = rect().width() / m_pixels_per_second;
            double scroll_max = old_scroll_right_pos - new_width_seconds;

            m_scroll_pos = old_scroll_pos + (scroll_max - old_scroll_pos) * lerp_val;

            //double pixels_per_sample = (m_pixels_per_second / (double) the_app.buffer.get_sample_rate());
            //qDebug() << "pixels/sample: " << pixels_per_sample << "samples/pixel: " << 1.0 / pixels_per_sample;

            update();
        }

        if (wheel->angleDelta().x() != 0) {
            m_scroll_pos -= wheel->angleDelta().x() / m_pixels_per_second;
            update();
        }

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

void AudioWidget::set_zoom(double zoom) {
	zoom = fmin(50, zoom);
	zoom = fmax(0.001, zoom);

	m_zoom = zoom;
	m_pixels_per_second = pow(1.5, m_zoom);
}

void AudioWidget::reset_view() {
	const int margin = 50;

	double zoom = 12;
	if (the_app.buffer.get_num_frames() > 0) {
		int pixel_width = rect().width() - margin * 2;
		double duration = the_app.buffer.get_duration();
		zoom = log(pixel_width / duration) / log(1.5);
	}
	set_zoom(zoom);
	m_scroll_pos = -margin / m_pixels_per_second;

	update();
}
