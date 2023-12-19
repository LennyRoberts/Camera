#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
  : QMainWindow(parent)
  , ui(new Ui::MainWindow)
{
  ui->setupUi(this);
  ui->label_img->setAlignment(Qt::AlignCenter);

  ui->spinBox_udpport->setValue(8888);
  ui->spinBox_tcpport->setValue(80);
  this->net_tcp = new TCPNet();
  this->net_udp = new UDPNet();
  connect(ui->button_tcpstart, &QPushButton::clicked, this, &MainWindow::TCPConnect);
  connect(ui->button_udpstart, &QPushButton::clicked, this, &MainWindow::UDPConnect);
  connect(ui->button_tcpstop, &QPushButton::clicked, this, &MainWindow::TCPBreak);
  connect(ui->button_udpstop, &QPushButton::clicked, this, &MainWindow::UDPBreak);
}

MainWindow::~MainWindow()
{
  delete ui;
}

void MainWindow::TCPConnect()
{
  int port = ui->spinBox_tcpport->value();
  this->net_tcp->Connect(port);
}

void MainWindow::TCPBreak()
{

}

void MainWindow::UDPConnect()
{
  int port = ui->spinBox_udpport->value();
  qDebug() << "udp port = " << port;
  this->net_udp->Connect(port);
  connect(this->net_udp, &UDPNet::display_img,
          this, &MainWindow::ShowImg);
  ui->button_udpstart->setEnabled(false);
  ui->button_tcpstop->setEnabled(true);
}

void MainWindow::UDPBreak()
{
  this->net_udp->Close();
  disconnect(this->net_udp, &UDPNet::display_img,
             this, &MainWindow::ShowImg);
  ui->button_udpstart->setEnabled(true);
  ui->button_tcpstop->setEnabled(false);
}

void MainWindow::ShowImg(QByteArray &array, qint64 size)
{
  qDebug() << "image_size = " << size << "\n";
  this->image.loadFromData(array);
  this->image = this->image.scaled(640, 480);
  ui->label_img->setPixmap(QPixmap::fromImage(this->image));
}





