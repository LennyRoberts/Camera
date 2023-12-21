#ifndef NETWORK_H
#define NETWORK_H

#include <QObject>
#include <QByteArray>
#include <QUdpSocket>

#include <errno.h>
#include <sys/mman.h>
#include <unistd.h>
#include <sys/select.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <cstdio>
#include <cstdlib>

#define INVALID_FD  -1

class UDPNet : public QObject {
  Q_OBJECT
 public:
  UDPNet();
  ~UDPNet();
  bool Connect(int port);
  void Close();

 signals:
  void display_img(QByteArray &array, qint64 size);

 public slots:
  void processPendingDatagrams();

 public:
  QUdpSocket  *udp_sock;
 private:
  int port;
  QByteArray     data_image;
  qint64         data_size;
  qint64         recv_index;
};

class TCPNet : public QObject  {
  Q_OBJECT
 public:
  TCPNet();
  ~TCPNet();
  bool Connect(int port);
  int Send(char *buff, int len);
  int Recv(char *buff, int len);
  void Close();

 public slots:


 private:
  int client_fd;
  int server_fd;
  int  port;
};

#endif // NETWORK_H
