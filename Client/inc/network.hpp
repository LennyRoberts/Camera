#ifndef NETWORK_HPP
#define NETWORK_HPP

#include <iostream>
#include <string>
#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <cstring>
#include <mutex>

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

const int size_pack_udp = 0x8000; //一个UDP包最大长度：60KB

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

class TCPNet {
 private:
  int sock_fd;
  struct sockaddr_in addr_in;
 public:
  bool     en_bit;
  NetStatu net_stu;
 public:
  TCPNet(){
    this->en_bit = false;
    this->net_stu = NetStatu::StatuRunFail;
    this->sock_fd = INVALID_FD;
    bzero(&this->addr_in, sizeof(struct sockaddr_in));
  }
  ~TCPNet(){}

  std::string GetAddrs(){
    return (std::string)inet_ntoa(this->addr_in.sin_addr);
  }
  int GetPort(){return (int)ntohl(this->addr_in.sin_port);}

  bool Connect(std::string addrs, int port)
  {
    this->sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if(this->sock_fd < 0){
      perror("tcp socket error");
      this->sock_fd = INVALID_FD;
      return false;
    }
    addr_in.sin_family = AF_INET;
    inet_aton(addrs.c_str(), &addr_in.sin_addr);
    addr_in.sin_port   = htons(port);
    while(1){
      int res = connect(this->sock_fd, (struct sockaddr*)&addr_in, sizeof(addr_in));
      if(res == 0){
        this->net_stu = NetStatu::StatuLink;
        break;
      }
    }
    return true;
  }

  int Recv(char *buff, int len)
  {
    int size = 0;
    while(1){
      size = recv(this->sock_fd, buff, len, 0);
      if(size > 0)
        break;
    }
    return size;
  }
  
  int Send(const char *buff, int len)
  {
    int size = send(this->sock_fd, buff, len, 0);
    return size;
  }

  void Close()
  {
    shutdown(this->sock_fd, SHUT_RDWR);
    close(this->sock_fd);
    this->sock_fd = INVALID_FD;
    this->net_stu = NetStatu::StatuRunFail;
  }
};

class UDPNet {
 private:
  int sock_fd;
  struct sockaddr_in addr_in;
  std::mutex  mutex_send;
 public:
  bool        en_bit;
  NetStatu    net_stu;
public:
  UDPNet(){
    this->sock_fd = INVALID_FD;
    this->net_stu = NetStatu::StatuRunFail;
    this->en_bit  = false;
  }
  ~UDPNet(){}
  bool isNetEnable() {return this->en_bit;}
  void EnableNet() {this->en_bit = true;}
  void DisnableNet() {this->en_bit = false;}
  void SetNetStatus(enum NetStatu status) {this->net_stu = status;}
  enum NetStatu GetNetStatus() { return this->net_stu; }
  bool isNetStatus(NetStatu sta){
    if(this->net_stu == sta) return true;
    else return false;
  }

  bool Connect(std::string addrs, int port)
  {
    this->sock_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if(this->sock_fd < 0){
      perror("UDP Socket Error!!!\n");
      return false;
    }
    memset(&this->addr_in, 0, sizeof(this->addr_in));
    this->addr_in.sin_family = AF_INET;
    inet_aton(addrs.c_str(), &addr_in.sin_addr);
    this->addr_in.sin_port = htons(port);
    this->net_stu = NetStatu::StatuLink;
    return true;
  }

  int Recv(char *buff, int len)
  {
    struct sockaddr* sock_addr = (struct sockaddr*)&this->addr_in;
    int addr_len = sizeof(*sock_addr);
    int size = recvfrom(this->sock_fd, buff, len, 0, sock_addr, (socklen_t*)&addr_len);
    return size;
  }

  int Send(const char *buff, int len)
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
      size = sendto(this->sock_fd, tmp_buff, send_size+2, 0, sock_addr, addr_len);
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

  int SendImg(const char *buff, int len, 
              int cam_index, int fmt_index) 
  {
    std::lock_guard<std::mutex> lock(this->mutex_send);
    struct sockaddr* sock_addr = (struct sockaddr*)&this->addr_in;
    int addr_len = sizeof(*sock_addr);
    char tmp_buff[size_pack_udp+4];
    int  offset  = 0;
    int  size    = 0;
    int  index   = 1;
    int  num_packet = len / size_pack_udp;
    if(len % size_pack_udp > 0)
      num_packet++;
    while(offset < len){
      int send_size = std::min(size_pack_udp, len-offset);
      memset(tmp_buff, '\0', size_pack_udp+4);
      tmp_buff[0] = index&0xFF;       /*分包序号*/
      tmp_buff[1] = num_packet&0xFF;  /*分包数量*/
      tmp_buff[2] = cam_index&0xFF;   /*相机序号*/
      tmp_buff[3] = fmt_index&0xFF;   /*相机参数序号*/
      memcpy(tmp_buff+4, buff+offset, send_size);
      size = sendto(this->sock_fd, tmp_buff, send_size+4, 0, sock_addr, addr_len);
      if(size > 0){
        #if 1 //PRINT_NETWORK
        printf("send %dth[num=%d] size=%d\n", index,num_packet, size);
        #endif
      } else {
        #if PRINT_NETWORK
        printf("send %dth part fail!!!---> ", index);
        #endif
        perror("send fail");
        return -1;
      }
      offset += send_size;
      index++;
    }
    return offset;
  }

  void Close()
  {
    close(this->sock_fd);
    this->sock_fd = INVALID_FD;
  }
};

#endif