#include "network.h"

UDPNet::UDPNet(){this->port = 8888;}
UDPNet::~UDPNet(){}

bool UDPNet::Connect(int port)
{
  this->udp_sock = new QUdpSocket();
  if(!this->udp_sock->bind(QHostAddress::Any, port)){
    qDebug() << "Error binding socket!!!";
    return false;
  }
  connect(this->udp_sock, &QUdpSocket::readyRead,
          this, &UDPNet::processPendingDatagrams);
  this->port = port;
  return true;
}

void UDPNet::Close()
{
  this->udp_sock->close();
  disconnect(this->udp_sock, &QUdpSocket::readyRead,
             this, &UDPNet::processPendingDatagrams);
}

void UDPNet::processPendingDatagrams()
{
  while(udp_sock->hasPendingDatagrams()){
    // 读取收到的数据
    QByteArray datagram;
    datagram.clear();
    datagram.resize(this->udp_sock->pendingDatagramSize());
    QHostAddress sender;
    quint16 senderPort;
    qint64 size = this->udp_sock->readDatagram(datagram.data(), datagram.size(),
                                               &sender, &senderPort);
    if(size > 0){
//      qDebug() << "recv size= " << size;
//      qDebug() << "datagram[0]" << QString("%1").arg(datagram[0]);
//      qDebug() << "datagram[1]" << QString("%1").arg(datagram[1]);
      this->recv_index++;
      if(this->recv_index == datagram[1]){
        if(this->recv_index == datagram[0]){
          datagram.remove(0,2);
          this->data_image.append(datagram);
          this->data_size += (size-2);
          emit display_img(this->data_image, this->data_size);
          this->data_image.clear();
          this->data_size  = 0;
          this->recv_index = 0;
        } else {
          datagram.remove(0,2);
          this->data_image.append(datagram);
          this->data_size += (size-2);
        }
      } else {
        qDebug() << "lost packet " << this->recv_index;
        this->data_image.clear();
        this->data_size  = 0;
        this->recv_index = 0;
      }
    }
  }
}


TCPNet::TCPNet(){this->port = 80;}
TCPNet::~TCPNet(){}

bool TCPNet::Connect(int port)
{
  this->tcp_server = new QTcpServer();
  QObject::connect(this->tcp_server, &QTcpServer::newConnection,
                   this, &TCPNet::NewConnection);
  bool res = this->tcp_server->listen(QHostAddress::Any, port);
  if(res) {
    qDebug() << "connect tcp: ";
    this->port = port;
    return true;
  } else
    qDebug() << "connect fail";
  return false;
}


void TCPNet::NewConnection()
{
  this->tcp_sock = tcp_server->nextPendingConnection();
  QObject::connect(this->tcp_sock, &QTcpSocket::readyRead,
                   this, &TCPNet::ProcessData);
  qDebug() << "New connection from: " << this->tcp_sock->peerAddress().toString();
}

void TCPNet::ProcessData()
{
  QByteArray data = this->tcp_sock->readAll();
  qDebug() << "Recv data: " << data;
}


















