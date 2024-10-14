#ifndef AUDIOWIDGET_H
#define AUDIOWIDGET_H

#include <QWidget>

class AudioWidget : public QWidget {
    Q_OBJECT
public:
    explicit AudioWidget(QWidget *parent = nullptr);

    double get_mouse_pos() const { return m_mouse_pos; }

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

    State state = State::IDLE;
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

    void paintEvent(QPaintEvent *event);
    bool event(QEvent *event);
    int project_x(double time);
    int project_y(float amplitude);

    friend class MainWindow;
};

#endif // AUDIOWIDGET_H
