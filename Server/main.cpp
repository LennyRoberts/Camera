#include <QApplication>

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "network.h"

int main(int argc, char *argv[])
{
  QApplication app(argc, argv);
  MainWindow *m_wnd = new MainWindow;
  m_wnd->setWindowTitle("camera");
  m_wnd->setWindowIcon(QIcon(":/src/img/camera.png"));
  m_wnd->show();

  return app.exec();
}
