#include "network.h"

NetWork::NetWork()
{

}

NetWork::~NetWork()
{

}

int NetWork::SocketTCP()
{
  int fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if(fd < 0){
    perror("socket error");
  }
  return fd;
}

bool NetWork::Bind(int fd, std::string addrs, int port)
{
  struct sockaddr_in server_addr;
  bzero(&server_addr, sizeof(struct sockaddr_in));
  server_addr.sin_family = AF_INET;
  //  if(addrs.empty()){
  /*数值转网络字节序用hton**/
  server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  //  } else {
  //    /*字符串转网络字节序用inet_**/
  //    char buff[16];
  //    memcpy(buff, addrs.c_str(),16);
  //    inet_aton(buff, &server_addr.sin_addr);
  //  }
  server_addr.sin_port = htons(port);
  // inet_pton(AF_INET, "8889", &server_addr.sin_port);
  int res = bind(fd, (const sockaddr*)&server_addr,
                 (socklen_t)sizeof(server_addr));
  if(res < 0){
    perror("bind error");
    qDebug() << "errno=" << errno;
    return false;
  }
  return true;
}

bool NetWork::Connet(int fd, std::string addrs, int port)
{
  struct sockaddr_in client_addr;
  bzero(&client_addr, sizeof(struct sockaddr_in));
  client_addr.sin_family = AF_INET;
  if(addrs.empty()){
    /*数值转网络字节序用hton**/
    client_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  } else {
    /*字符串转网络字节序用inet_**/
    char buff[16];
    memcpy(buff, addrs.c_str(),16);
    inet_aton(buff, &client_addr.sin_addr);
  }
  client_addr.sin_addr.s_addr = htonl(port);
  // inet_pton(AF_INET, "8889", &client_addr.sin_port);
  while(1){
    int res = ::connect(fd, (struct sockaddr*)&client_addr,
                        (socklen_t)sizeof(struct sockaddr_in));
    if(res < 0){
      continue;
    } else {

    }
    return true;
  }
}

bool NetWork::Listen(int fd)
{
  int res = listen(fd, 5);
  if(res < 0){
    perror("Listen error");
    return false;
  }
  return true;
}

int NetWork::Accept(int fd, struct sockaddr_in *client_addr,
                    socklen_t *addr_len)
{
  while(1){
    int client_fd = accept(fd, (struct sockaddr*)&client_addr, addr_len);
    if(client_fd < 0){
      continue;
    }
    return client_fd;
  }
}


void NetWork::CloseSock(int fd)
{
  shutdown(fd, SHUT_RDWR);
  close(fd);
}

int NetWork::SendMsg(int fd, char* buff, int size)
{
  int send_size = ::send(fd, buff, size, 0);
  if(send_size < 0){
    perror("send error");
  }
  return send_size;
}

int NetWork::RecvMsg(int fd, char* buff, int size)
{
  int recv_size = ::recv(fd, buff, size, 0);
  if(recv_size < 0){
    perror("recv error");
  }
  return recv_size;
}



/*============================================================================
 *
 *============================================================================*/

UDPNet::UDPNet(){this->port = 8888;}
UDPNet::~UDPNet(){}

bool UDPNet::Connect(quint16 port)
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
      int recv_index = datagram[0];
      int pack_num   = datagram[1];
      int cam_index  = datagram[2];
      int fmt_index  = datagram[3];
      datagram.remove(0,4);
      this->recv_index++;
      if(this->recv_index == recv_index){
        if(this->recv_index == pack_num){
          this->data_image.append(datagram);
          this->data_size += (size-4);
          emit display_img(this->data_image, this->data_size);
          this->data_image.clear();
          this->data_size  = 0;
          this->recv_index = 0;
        } else {
          this->data_image.append(datagram);
          this->data_size += (size-4);
        }
      } else {
        this->data_image.clear();
        this->data_size  = 0;
        this->recv_index = 0;
      }
    }
  }
}

/*============================================================================
 *
 *============================================================================*/
TCPNetServer::TCPNetServer(){}
TCPNetServer::TCPNetServer(QString addrs, quint16 port)
{
  this->addrs = addrs;
  this->port  = port;
}
TCPNetServer::~TCPNetServer(){}
void TCPNetServer::Listen()
{
  this->tcp_server = new QTcpServer();
  bool res = this->tcp_server->listen(QHostAddress::Any, this->port);
  if(res){
    QObject::connect(this->tcp_server, &QTcpServer::newConnection,
                     this, &TCPNetServer::ConnectToClient);
  } else
    QMessageBox::information(NULL, "TCP Connect", "TCP listen fail!");
}

void TCPNetServer::ConnectToClient()
{
  this->tcp_sock = this->tcp_server->nextPendingConnection();
  this->addrs = this->tcp_sock->peerAddress().toString().split("::ffff:")[1];
  this->port = this->tcp_sock->peerPort();
//  QMessageBox::information(NULL, "TCP Connect",
//                           "Accept client is " + addrs + ":" +
//                           QString::number(port, 10));
  QObject::connect(this->tcp_sock, &QTcpSocket::readyRead,
                   this, &TCPNetServer::ReadInformation);
  emit sig_connstu(true, this->addrs, this->port);
}

void TCPNetServer::ReadInformation()
{
  QByteArray tcp_data = this->tcp_sock->readAll();
  emit sig_tcp_data(tcp_data, tcp_data.size());
}

void TCPNetServer::Close()
{
  this->tcp_server->close();
  this->tcp_sock->close();
  QObject::disconnect(this->tcp_sock, &QTcpSocket::readyRead,
                      this, &TCPNetServer::ReadInformation);
  QObject::disconnect(this->tcp_server, &QTcpServer::newConnection,
                      this, &TCPNetServer::ConnectToClient);
}

void TCPNetServer::SendDataTCP(QByteArray array, quint64 size)
{
  this->tcp_sock->write(array, size);
}


/*============================================================================
 *
 *============================================================================*/
#if 0
TCPNet::TCPNet()
{
  this->server_fd = INVALID_FD;
  this->client_fd = INVALID_FD;
  this->net_stu   = NetStatu::StatuRunFail;
}

TCPNet::TCPNet(std::string addrs, int port)
{
  this->addrs = addrs;
  this->port  = port;
  this->server_fd = INVALID_FD;
  this->client_fd = INVALID_FD;
  this->net_stu = NetStatu::StatuRunFail;
}

TCPNet::~TCPNet()
{
}


bool TCPNet::Connect(std::string addrs, int port)
{
  this->server_fd = SocketTCP();
  if(this->server_fd < 0)
    return false;
  if(!Bind(this->server_fd, addrs, port))
    return false;
  if(!Listen(this->server_fd))
    return false;
  socklen_t *sockaddr_len;
  bzero(&client_addr, sizeof(struct sockaddr_in));
  this->client_fd = Accept(this->server_fd, &this->client_addr, sockaddr_len);
  if(this->client_fd < 0)
    return false;
  this->client_addrs = inet_ntoa(this->client_addr.sin_addr);
  return true;
}


int TCPNet::Send(char *buff, int size)
{
  int send_size = SendMsg(this->client_fd, buff, size);
  return send_size;
}

int TCPNet::Recv(char *buff, int size)
{
  int recv_size = RecvMsg(this->client_fd, buff, size);
  return recv_size;
}


void TCPNet::Close()
{
  CloseSock(this->server_fd);
  CloseSock(this->client_fd);
}
#endif








