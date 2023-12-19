#ifndef NETWORK_H
#define NETWORK_H

#include <QObject>
#include <QByteArray>
#include <QUdpSocket>
#include <QTcpSocket>
#include <QTcpServer>

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

 public slots:
  void NewConnection();
  void ProcessData();

 public:
  QTcpSocket  *tcp_sock;
  QTcpServer  *tcp_server;
 private:
  int  port;
};

#endif // NETWORK_H
