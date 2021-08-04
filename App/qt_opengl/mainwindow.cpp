#include "mainwindow.h"
#include "mpvwidget.h"
#include <QPushButton>
#include <QSlider>
#include <QLayout>
#include <QFileDialog>
#include <QEvent>
#include <QKeyEvent>
#include <QDebug>
#include <QWheelEvent> 
#include <QHostAddress>
#include <QAbstractSocket>

MainWindow::MainWindow(QWidget *parent) : QWidget(parent), _server(this)
{
    m_mpv = new MpvWidget(this);
    m_mpv->installEventFilter(this);
    m_mpv->setFocusPolicy(Qt::StrongFocus);
    QHBoxLayout *hb = new QHBoxLayout();
    QVBoxLayout *vl = new QVBoxLayout();
    vl->addWidget(m_mpv);
    vl->addLayout(hb);
    setLayout(vl);
    m_mpv->command(QStringList() << "loadfile" << "tcp://0.0.0.0:12345?listen");
    _server.listen(QHostAddress::Any, 12346);
    connect(&_server, SIGNAL(newConnection()), this, SLOT(onNewConnection()));
}

void MainWindow::onNewConnection()
{
   qInfo()<<"onNewConnection()";
   QTcpSocket *clientSocket = _server.nextPendingConnection();
   connect(clientSocket, SIGNAL(readyRead()), this, SLOT(onReadyRead()));
   connect(clientSocket, SIGNAL(stateChanged(QAbstractSocket::SocketState)), this, SLOT(onSocketStateChanged(QAbstractSocket::SocketState)));

    _sockets.push_back(clientSocket);
    for (QTcpSocket* socket : _sockets) {
        socket->write(QByteArray::fromStdString(clientSocket->peerAddress().toString().toStdString() + " connected to server !\n"));
        qInfo()<<"connected to server!";
    }
}

void MainWindow::onSocketStateChanged(QAbstractSocket::SocketState socketState)
{
    qInfo()<<"onSocketStateChanged()";
    if (socketState == QAbstractSocket::UnconnectedState)
    {
        QTcpSocket* sender = static_cast<QTcpSocket*>(QObject::sender());
        _sockets.removeOne(sender);
    }
}

void MainWindow::onReadyRead()
{
    qInfo()<<"onReadyRead()";
    QTcpSocket* sender = static_cast<QTcpSocket*>(QObject::sender());
    QByteArray datas = sender->readAll();
    for (QTcpSocket* socket : _sockets) {
        if (socket != sender)
            socket->write(QByteArray::fromStdString(sender->peerAddress().toString().toStdString() + ": " + datas.toStdString()));
    }
}

bool MainWindow::sendMessage(QString msg)
{
    if (_sockets.count() > 0){
      msg += " ";
      _sockets[0]->write(msg.toUtf8().leftJustified(64, '.'));
      qInfo()<<msg<<"SENT";
      return true;
    }
    qInfo()<<msg<<"NOT SENT";
    return false;
}

QString MainWindow::buttonName(int num)
{
    switch (num)
    {
      case 1:
        return "Button.left";
      case 2:
        return "Button.right";
      case 3:
        return "Button.middle";
      case 4:
        return "Button.middle";
      default:
        return "Button.left";
    }
}

QString MainWindow::keyStr(int code, QString text)
{
  qInfo()<<QString("keyStr(%1)").arg(code);
  switch (code)
  {
    case 32:
      return QString("Key.space");
    case 16777216:
      return QString("Key.esc");
    case 16777217:
      return QString("Key.tab");
    case 16777219:
      return QString("Key.backspace");
    case 16777220:
      return QString("Key.enter");
    case 16777234:
      return QString("Key.left");
    case 16777235:
      return QString("Key.up");
    case 16777236:
      return QString("Key.right");
    case 16777237:
      return QString("Key.down");
    case 16777248:
      return QString("Key.shift_l");
    case 16777249:
      return QString("Key.ctrl_l");
    case 16777299:
      return QString("Key.cmd_l");
    case 16777251:
      return QString("Key.alt_l");
    case 16781571:
      return QString("Key.alt_r");
    default:
      return QString("%1").arg(text);
  }
}

bool MainWindow::eventFilter(QObject *obj, QEvent *event)
{
  // qInfo()<<event->type();

  if (event->type() == QEvent::KeyPress)
  {
    QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);
    QString msg = QString("keyboard press %1").arg(keyStr(keyEvent->key(), keyEvent->text()));
    sendMessage(msg);
  }
  else if (event->type() == QEvent::KeyRelease)
  {
    QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);
    QString msg = QString("keyboard release %1").arg(keyStr(keyEvent->key(), keyEvent->text()));
    sendMessage(msg);
  }

  else if (event->type() == QEvent::MouseMove)
  {
    QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
    QString msg = QString("mouse %1 %2 move").arg(mouseEvent->pos().x()).arg(mouseEvent->pos().y());
    sendMessage(msg);
  }
  else if (event->type() == QEvent::MouseButtonPress)
  {
    QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
    QString msg = QString("mouse %1 %2 click %3").arg(mouseEvent->pos().x()).arg(mouseEvent->pos().y()).arg(buttonName(mouseEvent->button()));
    sendMessage(msg);
  }
  else if (event->type() == QEvent::MouseButtonRelease)
  {
    QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
    QString msg = QString("mouse %1 %2 release %3").arg(mouseEvent->pos().x()).arg(mouseEvent->pos().y()).arg(buttonName(mouseEvent->button()));
    sendMessage(msg);
  }

  else if (event->type() == QEvent::Wheel)
  {
    QWheelEvent *wheelEvent = static_cast<QWheelEvent*>(event);
    int dx = (-1 * -wheelEvent->angleDelta().x())/(wheelEvent->angleDelta().x() * -1); // preserve sign, turn into +1 or -1
    int dy = (-1 * -wheelEvent->angleDelta().y())/(wheelEvent->angleDelta().y() * -1); // preserve sign, turn into +1 or -1
    QString msg = QString("mouse %1 %2 scroll %3 %4")
      .arg(wheelEvent->pos().x())
      .arg(wheelEvent->pos().y())
      .arg(dx)
      .arg(dy);
    sendMessage(msg);
  }
   
  return false;
}
