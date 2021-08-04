#ifndef MainWindow_H
#define MainWindow_H

#include <QtWidgets/QWidget>
#include <QEvent>

class MpvWidget;
class QSlider;
class QPushButton;
class MainWindow : public QWidget
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = 0);
public Q_SLOTS:
    void openMedia();
    void seek(int pos);
    void pauseResume();
    bool eventFilter(QObject *obj, QEvent *event);
private Q_SLOTS:
private:
    MpvWidget *m_mpv;
    QSlider *m_slider;
    QPushButton *m_openBtn;
    QPushButton *m_playBtn;
};

#endif // MainWindow_H
