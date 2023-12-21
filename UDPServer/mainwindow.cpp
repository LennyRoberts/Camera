#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
  : QMainWindow(parent),
    ui(new Ui::MainWindow)
{
  ui->setupUi(this);
  ui->label_img->setAlignment(Qt::AlignCenter);
  ui->spinBox_udpport->setValue(8888);
  ui->spinBox_tcpport->setValue(8889);
  this->net_tcp = new TCPNet();
  this->net_udp = new UDPNet();
  ui->button_udpstop->setEnabled(false);
  ui->button_tcpstop->setEnabled(false);
  connect(ui->button_tcpstart, &QPushButton::clicked, this, &MainWindow::TCPConnect);
  connect(ui->button_udpstart, &QPushButton::clicked, this, &MainWindow::UDPConnect);
  connect(ui->button_tcpstop,  &QPushButton::clicked, this, &MainWindow::TCPBreak);
  connect(ui->button_udpstop,  &QPushButton::clicked, this, &MainWindow::UDPBreak);
}

MainWindow::~MainWindow()
{
  delete ui;
}

void MainWindow::TCPConnect()
{
  ui->button_tcpstart->setEnabled(false);
  ui->button_tcpstop->setEnabled(true);
  int port = ui->spinBox_tcpport->value();
  this->tcp_thread = new MyThread(this->net_tcp);
  if(this->net_tcp->Connect(port)){
    connect(this->tcp_thread, &MyThread::RecvTCPData,
            this, &MainWindow::ProcessTCPData);
    tcp_thread->start();
  } else {
    ui->button_tcpstart->setEnabled(true);
    ui->button_tcpstop->setEnabled(false);
  }
}

void MainWindow::TCPBreak()
{
  this->net_tcp->Close();
  this->tcp_thread->exit();
  disconnect(this->tcp_thread, &MyThread::RecvTCPData,
          this, &MainWindow::ProcessTCPData);
  ui->button_tcpstart->setEnabled(true);
  ui->button_tcpstop->setEnabled(false);
}

void MainWindow::UDPConnect()
{
  int port = ui->spinBox_udpport->value();
  qDebug() << "udp port = " << port;
  if(this->net_udp->Connect(port)){
    connect(this->net_udp, &UDPNet::display_img,
            this, &MainWindow::ShowImg);
    ui->button_udpstart->setEnabled(false);
    ui->button_udpstop->setEnabled(true);
  }
}

void MainWindow::UDPBreak()
{
  this->net_udp->Close();
  disconnect(this->net_udp, &UDPNet::display_img,
             this, &MainWindow::ShowImg);
  ui->button_udpstart->setEnabled(true);
  ui->button_udpstop->setEnabled(false);
}

void MainWindow::ShowImg(QByteArray &array, qint64 size)
{
  qDebug() << "image_size = " << size << "\n";
  this->image.loadFromData(array);
  this->image = this->image.scaled(640, 480);
  ui->label_img->setPixmap(QPixmap::fromImage(this->image));
}

void MainWindow::ProcessTCPData(QString qstr)
{
  qDebug()<< "recv data: " << qstr;
}

void MainWindow::OpenCamera()
{
  char buff[8] = {0x01, 0x06, 0x02, 0x00, 0x01, 0x00, 0x00, 0x5F};
  this->net_tcp->Send(buff, 8);
}

void MainWindow::CloseCamera()
{
  char buff[8] = {0x01, 0x06, 0x02, 0x00, 0x02, 0x00, 0x00, 0x5F};
  this->net_tcp->Send(buff, 8);
}

MyThread::MyThread(TCPNet *tcp)
{
  this->tcp = tcp;
}

void MyThread::run()
{
  while(1){
    char buff[65535];
    memset(buff, '\0', 65535);
    int size = this->tcp->Recv(buff, sizeof(buff));
    if(size > 0){
      QString qstr = QString(buff);
      emit RecvTCPData(qstr);
    }
  }
}



