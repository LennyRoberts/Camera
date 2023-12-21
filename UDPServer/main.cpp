#include "mainwindow.h"

#include <QApplication>
#include <QByteArray>
#include <QDebug>
#include <QIcon>
#include <QtGlobal>

#include "network.h"

int main(int argc, char *argv[])
{
  QApplication app(argc, argv);
  MainWindow win;
  win.setWindowTitle("camera");
  win.setWindowIcon(QIcon(":/src/img/camera.png"));
  win.show();  

  return app.exec();
}
