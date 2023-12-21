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


TCPNet::TCPNet()
{
  this->port = 80;
  this->server_fd = INVALID_FD;
  this->client_fd = INVALID_FD;
}
TCPNet::~TCPNet(){}

//bool TCPNet::Connect(int port)
//{
//  this->tcp_server = new QTcpServer();
//  bool res = this->tcp_server->listen(QHostAddress::Any, port);
//  if(res) {
//    qDebug() << "connect tcp: ";
//    this->port = port;
//    QObject::connect(this->tcp_server, &QTcpServer::newConnection,
//                     this, &TCPNet::NewConnection);
//    return true;
//  } else
//    qDebug() << "connect fail";
//  return false;
//}


bool TCPNet::Connect(int port)
{
  int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
  if(sock_fd < 0){
    perror("Socket Error");
    return false;
  }
  struct sockaddr_in server_in, client_in;
  server_in.sin_family      = AF_INET;
  server_in.sin_addr.s_addr = INADDR_ANY;
  server_in.sin_port        = htons(port);
  bzero(&(server_in.sin_zero), 8);
  socklen_t client_addrs_len = sizeof(client_in);
  if(bind(sock_fd, (struct sockaddr*)&server_in, sizeof(server_in)) < 0){
    perror("Bind Error");
    return false;
  }
  if(listen(sock_fd, 10) < 0){
    perror("Listen Error");
    return false;
  }
  this->server_fd = sock_fd;
  sock_fd = INVALID_FD;
  while(1){
    sock_fd = accept(this->server_fd,
                     (struct sockaddr*)&client_in,
                     &client_addrs_len);
    if(sock_fd < 0){
      continue;
    }
    this->client_fd = sock_fd;
    qDebug() << "accept connection from " <<
                inet_ntoa(client_in.sin_addr) <<
                htonl(client_in.sin_port);

    char buff[64];
    int size = this->Recv(buff, sizeof(buff));
    if(size <= 0){
      qDebug() << "Recv Test Data Error, Close!\n";
    } else
      qDebug() << "Recv Data: " << buff;
    size = this->Send(buff, size);
    if(size <= 0){
      qDebug() << "Send buff error!!!";
    } else {
      qDebug() << "Send Data: " << buff;
    }

    break;
  }
  return true;
}

int TCPNet::Send(char *array, int len)
{
  int size = send(this->client_fd, array, len, 0);
  return size;
}


int TCPNet::Recv(char *buff, int len)
{
  int size = 0;
  while(1){
    size = recv(this->client_fd, buff, len, 0);
    if(size > 0)
      break;
  }
  return size;
}

void TCPNet::Close()
{
  close(this->client_fd);
  shutdown(this->server_fd, SHUT_RDWR);
  close(this->server_fd);
  this->client_fd = INVALID_FD;
  this->server_fd = INVALID_FD;
}


















