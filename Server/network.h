#ifndef NETWORK_H
#define NETWORK_H

#include <QObject>
#include <QByteArray>
#include <QUdpSocket>
#include <QTcpServer>
#include <QTcpSocket>
#include <QMessageBox>

#include <sys/mman.h>
#include <iostream>
#include <string>
#include <cstdio>
#include <cstdlib>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <time.h>

#include <sys/types.h>   /*重新定义了内建类型名称*/
#include <netinet/in.h>  /*网际套接字地址结构: sockaddr_in,sockaddr_in6*/
#include <arpa/inet.h>   /*定义了网络字节序的转换[inet_*]*/
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <sys/select.h>

#define INVALID_FD  -1

/*Network Connection Status List*/
enum NetStatu {
  StatuRunFail = -1,
  StatuRunNull,
  StatuLink,
  StatuRecvStart,
  StatuParseMsg,
  StatuRecving,
  StatuRecvHandle,
  StstuRecvResponse,
  StatuRecvSuccess,
  StatuNeedSend,
  StatuSendStart,
  StatuSending,
  StatuSendPrepare,
  StatuSendSuccess,
  StatuSendWaitResponse,
  StatuResponFail,
  StatuSendFail,
  StatuSendUD,
  StatuRecvDspUD,
  StatuRecvArmUD,
  StatuMoSendFile
};

class NetWork : public QObject {
  Q_OBJECT
 public:
  NetWork();
  ~NetWork();
  int  SocketTCP();
  bool Bind(int fd, std::string addrs, int port);
  bool Connet(int fd, std::string addrs, int port);
  bool Listen(int fd);
  int  Accept(int fd, struct sockaddr_in *client_addr,
             socklen_t *addr_len);
  void CloseSock(int fd);
 public:
  int SendMsg(int fd, char* buff, int size);
  int SendMsg(int fd, char* buff, int size, struct sockaddr *sockaddr);
  int RecvMsg(int fd, char* buff, int size);
  int RecvMsg(int fd, char* buff, int size, struct sockaddr *sockaddr);
};


class UDPNet : public QObject {
  Q_OBJECT
 public:
  UDPNet();
  ~UDPNet();
  bool Connect(quint16 port);
  void Close();

 signals:
  void display_img(QByteArray &array, qint64 size);

 public slots:
  void processPendingDatagrams();

 public:
  QUdpSocket  *udp_sock;
  quint16 port;
 private:
  QByteArray     data_image;
  qint64         data_size;
  qint64         recv_index;
};

class TCPNetServer : public QObject {
  Q_OBJECT
 private:

 public:
  QTcpServer *tcp_server; /*用于监听*/
  QTcpSocket *tcp_sock;   /*用于同客户端交互*/
  QString    addrs;
  quint16    port;

 public:
  TCPNetServer();
  TCPNetServer(QString addrs, quint16 port);
  ~TCPNetServer();
 signals:
  void sig_connstu(bool stu, QString addrs, quint16 port);
  void sig_tcp_data(QByteArray &array, qint64 size);

 public slots:
  void Listen();
  void ConnectToClient();
  void ReadInformation();
  void Close();
  void SendDataTCP(QByteArray array, quint64 size);
};

//class TCPNetClient : public QObject {
//  Q_OBJECT
// private:
//  QTcpSocket *tcp_sock;
//  QString    addrs;
//  quint16    port;

// public:
//  TCPNetClient();
//  TCPNetClient(QString addrs, quint16 port);
//  ~TCPNetClient();
//};

//class TCPNet : public NetWork {
//  Q_OBJECT
// private:
//  int sock_fd;
//  int server_fd;
//  int client_fd;
//  struct sockaddr_in client_addr;
// public:
//  std::string client_addrs;
//  std::string addrs;
//  int port;
//  NetStatu net_stu;
// public:
//  TCPNet();
//  TCPNet(std::string addrs, int port);
//  ~TCPNet();
//  bool Connect(std::string addrs, int port);
//  int  Send(char *buff, int size);
//  int  Recv(char *buff, int size);
//  void Close();
//};

#endif // NETWORK_H
