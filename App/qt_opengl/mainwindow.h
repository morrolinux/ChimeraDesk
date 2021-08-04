#ifndef MainWindow_H
#define MainWindow_H

#include <QtWidgets/QWidget>
#include <QEvent>
#include <QTcpServer>
#include <QTcpSocket>

class MpvWidget;
class QSlider;
class QPushButton;
class MainWindow : public QWidget
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = 0);
public Q_SLOTS:
    bool eventFilter(QObject *obj, QEvent *event);
public slots:
    void onNewConnection();
    void onSocketStateChanged(QAbstractSocket::SocketState socketState);
    void onReadyRead();
private Q_SLOTS:
private:
    bool sendMessage(QString msg);
    QString buttonName(int num);
    QString keyStr(int code, QString text);
    QPoint translateMouseCoords(QPoint mp);
    MpvWidget *m_mpv;
    QTcpServer  _server;
    QList<QTcpSocket*>  _sockets;
};

#endif // MainWindow_H
