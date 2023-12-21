#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QPixmap>
#include <QImage>
#include <QPushButton>
#include <QThread>
#include <QString>

#include <iostream>
#include <cstdio>
#include <cstdlib>

#include "network.h"

class MyThread : public QThread {
  Q_OBJECT
 private:
  TCPNet *tcp;
 public:
  MyThread(TCPNet *tcp);
  void run() override;
 signals:
  void RecvTCPData(QString qstr);

};

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
  void ProcessTCPData(QString qstr);
  void TCPBreak();
  void UDPConnect();
  void UDPBreak();

  void OpenCamera();
  void CloseCamera();

 private:
  Ui::MainWindow *ui;
  QImage         image;
  TCPNet         *net_tcp;
  UDPNet         *net_udp;
  MyThread       *tcp_thread;
};



#endif // MAINWINDOW_H
