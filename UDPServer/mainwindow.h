#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QPixmap>
#include <QImage>
#include <QPushButton>

#include "network.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow {
  Q_OBJECT
 public:
  MainWindow(QWidget *parent = nullptr);
  ~MainWindow();



public slots:
  void ShowImg(QByteArray &array, qint64 size);
  void TCPConnect();
  void TCPBreak();
  void UDPConnect();
  void UDPBreak();

 private:
  Ui::MainWindow *ui;
  QImage         image;
  TCPNet         *net_tcp;
  UDPNet         *net_udp;
};
#endif // MAINWINDOW_H
