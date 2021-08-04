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


MainWindow::MainWindow(QWidget *parent) : QWidget(parent)
{
    m_mpv = new MpvWidget(this);
    m_mpv->installEventFilter(this);
    m_mpv->setFocusPolicy(Qt::StrongFocus);
    m_openBtn = new QPushButton("Open");
    m_playBtn = new QPushButton("Pause");
    QHBoxLayout *hb = new QHBoxLayout();
    hb->addWidget(m_openBtn);
    hb->addWidget(m_playBtn);
    QVBoxLayout *vl = new QVBoxLayout();
    vl->addWidget(m_mpv);
    vl->addLayout(hb);
    setLayout(vl);
    connect(m_openBtn, SIGNAL(clicked()), SLOT(openMedia()));
}

bool MainWindow::eventFilter(QObject *obj, QEvent *event)
{
  qInfo()<<event->type();

  if (event->type() == QEvent::KeyPress)
  {
    QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);
    qInfo()<<(QString("Key Down (%1)").arg(keyEvent->text()));
  }
  else if (event->type() == QEvent::KeyRelease)
  {
    QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);
    qInfo()<<(QString("Key Up (%1)").arg(keyEvent->text()));
  }

  else if (event->type() == QEvent::MouseMove)
  {
    QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
    qInfo()<<(QString("Mouse move (%1,%2)").arg(mouseEvent->pos().x()).arg(mouseEvent->pos().y()));
  }
  else if (event->type() == QEvent::MouseButtonPress)
  {
    QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
    qInfo()<<(QString("Mouse Down(%1,%2)").arg(mouseEvent->pos().x()).arg(mouseEvent->pos().y()));
  }
  else if (event->type() == QEvent::MouseButtonRelease)
  {
    QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
    qInfo()<<(QString("Mouse Up(%1,%2)").arg(mouseEvent->pos().x()).arg(mouseEvent->pos().y()));
  }

  else if (event->type() == QEvent::Wheel)
  {
    QWheelEvent *wheelEvent = static_cast<QWheelEvent*>(event);
    qInfo()<<(QString("Wheel (%1,%2)").arg(wheelEvent->angleDelta().x()).arg(wheelEvent->angleDelta().y()));
  }
   
  return false;
}

void MainWindow::openMedia()
{
    m_mpv->command(QStringList() << "loadfile" << "tcp://0.0.0.0:12345?listen");
    return;

    QString file = QFileDialog::getOpenFileName(0, "Open a video");
    if (file.isEmpty())
        return;
    m_mpv->command(QStringList() << "loadfile" << file);
}

void MainWindow::seek(int pos)
{
    m_mpv->command(QVariantList() << "seek" << pos << "absolute");
}

void MainWindow::pauseResume()
{
    const bool paused = m_mpv->getProperty("pause").toBool();
    m_mpv->setProperty("pause", !paused);
}
