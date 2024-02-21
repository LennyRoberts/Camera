#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
  : QMainWindow(parent),
    ui(new Ui::MainWindow)
{
  ui->setupUi(this);
  ui->label_img->setAlignment(Qt::AlignCenter);
  ui->spinBox_udpport->setValue(8090);
  ui->spinBox_tcpport->setValue(8888);
  this->net_tcp_ser = new TCPNetServer("", ui->spinBox_tcpport->value());
  this->net_udp = new UDPNet();
  UDPConnect();
  this->cam.reserve(CAMERA_NUM);

  QObject::connect(ui->btn_home, &QPushButton::clicked, this, &MainWindow::PageHome);
  QObject::connect(ui->btn_netset, &QPushButton::clicked, this, &MainWindow::PageNetWork);
  QObject::connect(ui->btn_camset, &QPushButton::clicked, this, &MainWindow::PageCamera);

  QObject::connect(ui->btn_connarm, &QPushButton::clicked,
                   this->net_tcp_ser, &TCPNetServer::Listen);
  QObject::connect(ui->btn_disconn, &QPushButton::clicked,
                   this->net_tcp_ser, &TCPNetServer::Close);

  QObject::connect(this->net_tcp_ser, &TCPNetServer::sig_tcp_data,
                   this, &MainWindow::ProcessTCPData);
  QObject::connect(this, &MainWindow::sig_tcp_send,
                   this->net_tcp_ser, &TCPNetServer::SendDataTCP);

  QObject::connect(ui->btn_senddata, &QPushButton::clicked,
                   this, &MainWindow::SendTCPData);
  QObject::connect(ui->btn_opencam, &QPushButton::clicked,
                   this, &MainWindow::SendOpenCam);
  QObject::connect(ui->btn_startcap, &QPushButton::clicked,
                   this, &MainWindow::StartCapture);
  QObject::connect(ui->btn_closecam, &QPushButton::clicked,
                   this, &MainWindow::CloseCam);
  QObject::connect(ui->btn_closecam, &QPushButton::clicked,
                   ui->cbox_optfmt, &QComboBox::clear);
  QObject::connect(ui->cbox_optcam, &QComboBox::currentTextChanged,
                   this, &MainWindow::ShowCamFmt);

}

MainWindow::~MainWindow()
{
  delete ui;
}

/*窗口切换*/
void MainWindow::PageHome(){
  ui->stackedWidget->setCurrentWidget(ui->page_home);
}
void MainWindow::PageNetWork(){
  ui->stackedWidget->setCurrentWidget(ui->page_network);
}
void MainWindow::PageCamera(){
  ui->stackedWidget->setCurrentWidget(ui->page_camera);
}

void MainWindow::SendTCPData()
{
  QByteArray array = ui->textEdit_send->toPlainText().toLatin1();
  qint64 size = this->net_tcp_ser->tcp_sock->write(array, array.size());
  if(size > 0)
    qDebug() << "send data to client successfully";
}

void MainWindow::ProcessTCPData(QByteArray &array, qint64 size)
{
  QString tcp_data = array;
  qDebug() << tcp_data;
  std::string std_str = tcp_data.toStdString();
  neb::CJsonObject tcp_json(std_str);
  short cmd = 0;
  tcp_json.Get("cmd", cmd);
  switch(cmd) {
    case 0x0103:{
      std::string str_para;
      tcp_json.Get("cam_num", this->cam_num);
      tcp_json.Get("para", str_para);
      neb::CJsonObject cap_json(str_para);
      for(int cam_i = 0; cam_i < CAMERA_NUM; ++cam_i) {
        neb::CJsonObject json_cam;
        std::string cam_str = "cam" + std::to_string(cam_i);
        bool result = cap_json.Get(cam_str, json_cam);
        if(!result)
          continue;
        CameraParam cam_para;
        json_cam.Get("cam_index", cam_para.cam_index);
        json_cam.Get("driver", cam_para.driver);
        json_cam.Get("card", cam_para.card);
        json_cam.Get("bus_info", cam_para.bus_info);
        json_cam.Get("version", cam_para.version);
        json_cam.Get("num_fmt", cam_para.num_fmt);
        ui->cbox_optcam->addItem(QString::fromStdString(cam_para.card)); //cam add to combox
        for(int fmt_i = 0; fmt_i < cam_para.num_fmt; ++fmt_i) {
          neb::CJsonObject json_fmt;
          std::string fmt_str = "fmt" + std::to_string(fmt_i);
          result = json_cam.Get(fmt_str, json_fmt);
          if (!result)
            continue;
          CamFmt cam_fmt;
          json_fmt.Get("fmt_index", cam_fmt.fmt_index);
          json_fmt.Get("format", cam_fmt.format);
          json_fmt.Get("width", cam_fmt.width);
          json_fmt.Get("height", cam_fmt.height);
          cam_para.cfmt.push_back(cam_fmt);
        }
        this->cam.push_back(cam_para);
      }
      ui->cbox_optfmt->clear();
      QString str_cam = ui->cbox_optcam->currentText();
      std::vector<CameraParam>::iterator it;
      for(it=this->cam.begin(); it!=this->cam.end(); ++it){
        if(str_cam == QString::fromStdString((*it).card)){
          std::vector<CamFmt>::iterator it_fmt;
          for(it_fmt=(*it).cfmt.begin(); it_fmt!=(*it).cfmt.end(); ++it_fmt){
            QString qstr_fmt = QString::fromStdString((*it_fmt).format) + ":" +
                               QString::number((*it_fmt).width, 10) + "*" +
                               QString::number((*it_fmt).height, 10);
            ui->cbox_optfmt->insertItem((*it_fmt).fmt_index, qstr_fmt);
          }
        }
      }
      break;
    }
    case 0x0104:{
      bool res;
      tcp_json.Get("res", res);
      if(res)
        ui->textEdit_recv->setText("Camera Init OK!");
      else
        ui->textEdit_recv->setText("Camera Init fail!");
      break;
    }
  }
  return;
}

void MainWindow::UDPConnect()
{
  int port = ui->spinBox_udpport->value();
  qDebug() << "udp port = " << port;
  bool res = this->net_udp->Connect(port);;
  if(res){
    connect(this->net_udp, &UDPNet::display_img,
            this, &MainWindow::ShowImg);
  }
}

void MainWindow::UDPBreak()
{
  this->net_udp->Close();
  disconnect(this->net_udp, &UDPNet::display_img,
             this, &MainWindow::ShowImg);
}


void MainWindow::SendOpenCam()
{
  neb::CJsonObject tcp_json;
  tcp_json.Add("cmd", OPEN_CAM);
  std::string str_data = tcp_json.ToString();
  QByteArray q_array(str_data.c_str(), str_data.length());
  emit sig_tcp_send(q_array, q_array.size());
}

void MainWindow::ShowCamFmt()
{
  QString str_cam = ui->cbox_optcam->currentText();
  std::vector<CameraParam>::iterator it;
  for(it=this->cam.begin(); it!=this->cam.end(); ++it){
    if(str_cam == QString::fromStdString((*it).card)){
      std::vector<CamFmt>::iterator it_fmt;
      for(it_fmt=(*it).cfmt.begin(); it_fmt!=(*it).cfmt.end(); ++it_fmt){
        QString qstr_fmt = QString::fromStdString((*it_fmt).format) + ":" +
                           QString::number((*it_fmt).width, 10) + "*" +
                           QString::number((*it_fmt).height, 10);
        ui->cbox_optfmt->insertItem((*it_fmt).fmt_index, qstr_fmt);
      }
      return;
    }
  }
}

void MainWindow::StartCapture()
{
  int cur_cam = 0;
  QString str_cam = ui->cbox_optcam->currentText();
  int cur_fmt = ui->cbox_optfmt->currentIndex();
  std::vector<CameraParam>::iterator it;
  for(it=this->cam.begin(); it!=this->cam.end(); ++it){
    if(str_cam == QString::fromStdString((*it).card)){
      cur_cam = (*it).cam_index;
    }
  }
  neb::CJsonObject tcp_json;
  tcp_json.Add("cmd", START_CAPTURE);
  tcp_json.Add("cam_i", cur_cam);
  tcp_json.Add("fmt_i", cur_fmt);
  std::string str_data = tcp_json.ToString();
  QByteArray q_array(str_data.c_str(), str_data.length());
  emit sig_tcp_send(q_array, q_array.size());
}

void MainWindow::ShowImg(QByteArray &array, qint64 size)
{
  this->image.loadFromData(array);
  this->image = this->image.scaled(640, 480);
  ui->label_img->setPixmap(QPixmap::fromImage(this->image));
}

void MainWindow::CloseCam()
{
  int cur_cam = 0;
  QString str_cam = ui->cbox_optcam->currentText();
  std::vector<CameraParam>::iterator it;
  for(it=this->cam.begin(); it!=this->cam.end(); ++it){
    if(str_cam == QString::fromStdString((*it).card)){
      cur_cam = (*it).cam_index;
    }
  }
  neb::CJsonObject tcp_json;
  tcp_json.Add("cmd", CLOSE_CAM);
  tcp_json.Add("cam_i", cur_cam);
  std::string str_data = tcp_json.ToString();
  QByteArray q_array(str_data.c_str(), str_data.length());
  emit sig_tcp_send(q_array, q_array.size());
  this->cam.clear();
  ui->cbox_optcam->clear();
  ui->cbox_optfmt->clear();
}

std::string PixeFormatToString(unsigned int pixelformat)
{
  qDebug() << "pixelformat= " << pixelformat;
  char ch1 = pixelformat & 0xFF;
  char ch2 = (pixelformat >> 8) & 0xFF;
  char ch3 = (pixelformat >> 16) & 0xFF;
  char ch4 = (pixelformat >> 24) & 0xFF;
  char buff[32];
  memset(buff, '\0', 32);
  sprintf(buff, "%c%c%c%c", ch1, ch2, ch3, ch4);
  std::string str_pixelformat = buff;
  return str_pixelformat;
}


void MyThread::run()
{

}

