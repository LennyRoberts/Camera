#ifndef NETWORK_HPP
#define NETWORK_HPP

#include <iostream>
#include <string>
#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <cstring>
#include <sys/select.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#define INVALID_FD         -1
#define DEFAULT_ADDR       "127.0.0.1"
#define DEFAUL_PORT        8888
#define UDP_PACKET_SIZE    61400  //60KB:

class NetWork {
 public:
  virtual ~NetWork(){}
  virtual bool Connect() = 0;
  virtual int  Recv(char *buff, int len) = 0;
  virtual int  Send(const char *buff, int len) = 0;
  virtual void Close() = 0;
};

class TCPNetServer : public NetWork {
 private:
  int client_fd;
  int server_fd;
  int port;
  std::string addrs;
  struct sockaddr_in addr_in;
 public:
  TCPNetServer(){
    this->addrs = DEFAULT_ADDR;
    this->port  = DEFAUL_PORT;
  }
  TCPNetServer(std::string addrs, int port){
    this->server_fd  = INVALID_FD;
    this->client_fd  = INVALID_FD;
    this->addrs = addrs;
    this->port  = port;
  }
  ~TCPNetServer(){}
  bool Connect() override
  {
    int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if(sock_fd < 0){
      perror("Socket Error");
      return false;
    }
    struct sockaddr_in server_in, client_in;
    server_in.sin_family      = AF_INET;
    server_in.sin_addr.s_addr = INADDR_ANY;
    server_in.sin_port        = htons(this->port);
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
      printf("accept connection from %s:%d\n",
              inet_ntoa(client_in.sin_addr), htons(client_in.sin_port));
      this->client_fd = sock_fd;
      char buff[64];
      while(1){
        memset(buff, '\0', 64);
        int size = this->Recv(buff, sizeof(buff));
        if(size < 0){
          this->Close();
          printf("Recv Test Data Error, Close!\n");
          break;
        }
        this->Send(buff, size);
      }
    }
    return true;
  }
  int Recv(char *buff, int len) override
  {
    int size = recv(this->client_fd, buff, len, 0);
    return size;
  }
  int Send(const char *buff, int len) override
  {
    int size = send(this->client_fd, buff, len, 0);
    return size;
  }
  void Close() override
  {
    close(this->client_fd);
    this->client_fd = INVALID_FD;
    close(this->server_fd);
    this->server_fd = INVALID_FD;
  }
};

class TCPNet : public NetWork {
 private:
  std::string addrs;
  int port;
  int sock_fd;
 public:
  TCPNet(){
    this->addrs   = "192.168.254.120";
    this->port    = 80;
    this->sock_fd = INVALID_FD;
  }
  TCPNet(std::string addrs, int port){
    this->addrs   = addrs;
    this->port    = port;
    this->sock_fd = INVALID_FD;
  }
  ~TCPNet(){}

  bool Connect() override
  {
    this->sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if(this->sock_fd < 0){
      perror("tcp socket error");
      this->sock_fd = INVALID_FD;
      return false;
    }
    struct sockaddr_in addr_in;
    addr_in.sin_family = AF_INET;
    inet_aton(this->addrs.c_str(), &addr_in.sin_addr);
    addr_in.sin_port   = htons(this->port);

    while(1){
      int res = connect(this->sock_fd, (struct sockaddr*)&addr_in, sizeof(addr_in));
      if(res < 0){
        perror("tcp connet error");
        this->sock_fd = INVALID_FD;
        continue;
      }
      printf("tcp connect successfully! %s:%d\n",
              inet_ntoa(addr_in.sin_addr), (int)htonl(addr_in.sin_port));
      break;
    }
    char buff[64];
    memset(buff, '\0', 64);
    strcpy(buff, "hello camera!");
    int size = Send(buff, strlen(buff));
    if(size < 0){
      perror("send buff fail");
      this->Close();
      return false;
    }
    printf("send buff: %s\n", buff);
    size = Recv(buff, sizeof(buff));
    if(size < 0){
      perror("TCP recv fail");
      this->Close();
      return false;
    }
    printf("recv buff: %s\n", buff);
    return true;
  }

  int Recv(char *buff, int len) override
  {
    int size = 0;
    while(1){
      size = recv(this->sock_fd, buff, len, 0);
      if(size > 0)
        break;
    }
    return size;
  }
  
  int Send(const char *buff, int len) override
  {
    int size = send(this->sock_fd, buff, len, 0);
    return false;
  }

  void Close() override
  {
    shutdown(this->sock_fd, SHUT_RDWR);
    close(this->sock_fd);
    this->sock_fd = INVALID_FD;
  }

};

class UDPNet : public NetWork {
 private:
  int fd;
  int port;
  std::string addrs;
  struct sockaddr_in addr_in;
public:
  UDPNet(){
    this->fd    = INVALID_FD;
    this->addrs = DEFAULT_ADDR;
    this->port  = DEFAUL_PORT;
  }
  UDPNet(std::string addrs, int port){
    this->fd    = INVALID_FD;
    this->addrs = addrs;
    this->port  = port;
  }
  ~UDPNet(){}

  bool Connect() override
  {
    this->fd = socket(AF_INET, SOCK_DGRAM, 0);
    if(this->fd < 0){
      perror("UDP Socket Error!!!\n");
      return false;
    }
    memset(&this->addr_in, 0, sizeof(this->addr_in));
    this->addr_in.sin_family = AF_INET;
    inet_aton(this->addrs.c_str(), &addr_in.sin_addr);
    this->addr_in.sin_port = htons(this->port);
  }

  int Recv(char *buff, int len) override
  {
    struct sockaddr* sock_addr = (struct sockaddr*)&this->addr_in;
    int addr_len = sizeof(*sock_addr);
    int size = recvfrom(this->fd, buff, len, 0, sock_addr, (socklen_t*)&addr_len);
    return size;
  }

  int Send(const char *buff, int len) override
  {
    struct sockaddr* sock_addr = (struct sockaddr*)&this->addr_in;
    int addr_len = sizeof(*sock_addr);
    char tmp_buff[UDP_PACKET_SIZE+2];
    int  offset = 0;
    int  size   = 0;
    int  index  = 1;
    int  num_packet = len / UDP_PACKET_SIZE;
    if(len % UDP_PACKET_SIZE > 0)
      num_packet++;
    while(offset < len){
      int send_size = std::min(UDP_PACKET_SIZE, len-offset);
      memset(tmp_buff, '\0', UDP_PACKET_SIZE+2);
      memcpy(tmp_buff+2, buff+offset, send_size);
      tmp_buff[0] = num_packet;
      tmp_buff[1] = index;
      size = sendto(this->fd, tmp_buff, send_size+2, 0, sock_addr, addr_len);
      if(size > 0){
        printf("send %dth part(%d)\n", index, size);
      } else {
        printf("send %dth part fail!!!---> ", index);
        perror("send fail");
        return -1;
      }
      offset += send_size;
      index++;
    }
    return offset;
  }

  void Close() override
  {
    close(this->fd);
    fd = INVALID_FD;
  }
};






#endif