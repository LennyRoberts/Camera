#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QPixmap>
#include <QImage>
#include <QPushButton>
#include <QThread>
#include <QString>
#include <QComboBox>
#include <QVector>

#include <iostream>
#include <string>
#include <cstdio>
#include <vector>
#include <cstdlib>

#include "network.h"
#include "CJsonObject.hpp"

#define CAMERA_NUM  6

#define OPEN_CAM        0x0103
#define START_CAPTURE   0x0104
#define CLOSE_CAM       0x0105

class CamFmt {
 public:
  unsigned int fmt_index;
  unsigned int width;
  unsigned int height;
  std::string  format;
};

class CameraParam {
 public:
  std::string    driver;    /*@driver:   驱动程序模块的名称（例如“bttv”*/
  std::string    card;      /*@card:     卡的名称（例如“Hauppauge WinTV”）*/
  std::string    bus_info;  /*@bus_info: 总线的名称（例如“PCI:”+PCI_name（PCI_dev））*/
  std::string    version;   /*@version:  KERNEL_version*/
  unsigned short num_fmt;   /*@num_fmt:  相机支持格式数量*/
  unsigned short cam_index; /*@index:    相机编号 */
  std::vector<CamFmt> cfmt; /*@cfmt:     format and size*/
//  QVector<CamFmt> qfmt;     /*@qfmt:     format and size*/
};

class MyThread : public QThread {
  Q_OBJECT
 public:
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
  void UDPConnect();
  void UDPBreak();

 signals:
  void sig_tcp_conn();
  void sig_tcp_send(QByteArray array, quint64 size);

 public slots:
  void PageHome();
  void PageNetWork();
  void PageCamera();
  void ShowImg(QByteArray &array, qint64 size);
  void ProcessTCPData(QByteArray &array, qint64 size);
  void SendTCPData();

 public slots:
  void SendOpenCam();
  void ShowCamFmt();
  void StartCapture();
  void CloseCam();

 private:
  Ui::MainWindow   *ui;
  QImage           image;
  TCPNetServer     *net_tcp_ser;
  UDPNet           *net_udp;
  MyThread         *tcp_thread;
  QThread          *qthread;
  std::vector<CameraParam> cam;
  uint16           cam_num;
//  QVector<CameraParam> qcam;
};



#endif // MAINWINDOW_H
