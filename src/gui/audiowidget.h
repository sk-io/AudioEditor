#ifndef AUDIOWIDGET_H
#define AUDIOWIDGET_H

#include <QWidget>

class AudioWidget : public QWidget {
    Q_OBJECT
public:
    enum class ViewMode {
        OVERLAPPED,
        SPLIT_CHANNEL,
        SPECTRUM,
    };

    explicit AudioWidget(QWidget *parent = nullptr);

    double get_mouse_pos() const { return m_mouse_pos; }
    void select(double start, double end);
    void deselect();
    void set_view_mode(ViewMode mode);
    double get_selection_start_time() const;
    double get_selection_end_time() const;

signals:
private:
    enum class State {
        IDLE,
        SCROLLING,
        SELECTING,
        RESIZE_REGION,
        RESIZE_REGION_HOVER,
    };

    enum class SelectionState {
        DESELECTED,
        MARKER,
        REGION,
    };

    void paintEvent(QPaintEvent *event);
    void draw_waveform_channel(int channel, QPainter& painter, int x0, int x1, int y0, int y1, const QColor& color);
    void draw_single_view();
    void draw_split_view();
    void draw_timeline(int y0, int y1);
    bool event(QEvent *event);
    int project_x(double time) const;
    int project_y(float amplitude, int y0, int y1) const;

private:
    State m_state = State::IDLE;
    double m_scroll_pos = 0; // at left side, in seconds
    int m_mouse_x = 0; // relative to this widget
    double m_mouse_pos = 0;
    double m_pixels_per_second = 1000; // the visual size of a second in pixels
    double m_drag_start_scroll_pos = 0;
    double m_drag_start_mouse_x = 0;
    double m_selection_pos_a = 0; // can be to the left or right of b
    double m_selection_pos_b = 0;
    SelectionState m_selection_state = SelectionState::DESELECTED;
    bool m_resizing_a = true; // true for a, false for b
    ViewMode m_view = ViewMode::OVERLAPPED;
    int m_timeline_height = 20;

    friend class MainWindow;
};

#endif // AUDIOWIDGET_H
