#include <iostream>
#include <string>
#include <linux/videodev2.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <sys/select.h>
#include <time.h>
#include <fstream>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <map>
#include <pthread.h>

#define INVALID_FD        -1
#define NB_BUFFER          4
#define DEFAULT_ADDR       "127.0.0.1"
#define DEFAUL_PORT        8888
#define UDP_PACKET_SIZE    61400  //60KB:

int width = 0;
int height= 0;

class NetWork {
 public:
  virtual ~NetWork(){}
  virtual bool Connect() = 0;
  virtual int  Recv(char *buff, int len) = 0;
  virtual int  Send(const char *buff, int len) = 0;
  virtual void Close() = 0;
};

// class TCPNetServer : public NetWork {
//  private:
//   int client_fd;
//   int server_fd;
//   int port;
//   std::string addrs;
//   struct sockaddr_in addr_in;
//  public:
//   TCPNetServer(){
//     this->addrs = DEFAULT_ADDR;
//     this->port  = DEFAUL_PORT;
//   }
//   TCPNetServer(std::string addrs, int port){
//     this->server_fd  = INVALID_FD;
//     this->client_fd  = INVALID_FD;
//     this->addrs = addrs;
//     this->port  = port;
//   }
//   ~TCPNetServer(){}
//   bool Connect()
//   {
//     int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
//     if(sock_fd < 0){
//       perror("Socket Error");
//       return false;
//     }
//     struct sockaddr_in server_in, client_in;
//     server_in.sin_family      = AF_INET;
//     server_in.sin_addr.s_addr = INADDR_ANY;
//     server_in.sin_port        = htons(this->port);
//     bzero(&(server_in.sin_zero), 8);
//     socklen_t client_addrs_len = sizeof(client_in);
//     if(bind(sock_fd, (struct sockaddr*)&server_in, sizeof(server_in)) < 0){
//       perror("Bind Error");
//       return false;
//     }
//     if(listen(sock_fd, 10) < 0){
//       perror("Listen Error");
//       return false;
//     }
//     this->server_fd = sock_fd;
//     sock_fd = INVALID_FD;
//     while(1){
//       sock_fd = accept(this->server_fd, 
//                        (struct sockaddr*)&client_in, 
//                        &client_addrs_len);
//       if(sock_fd < 0){
//         continue;
//       }
//       printf("accept connection from %s:%d\n",
//               inet_ntoa(client_in.sin_addr), htons(client_in.sin_port));
//       this->client_fd = sock_fd;
//       char buff[64];
//       while(1){
//         memset(buff, '\0', 64);
//         int size = this->Recv(buff, sizeof(buff));
//         if(size < 0){
//           this->Close();
//           printf("Recv Test Data Error, Close!\n");
//           break;
//         }
//         this->Send(buff, size);
//       }
//     }
//     return true;
//   }
//   int Recv(char *buff, int len)
//   {
//     int size = recv(this->client_fd, buff, len, 0);
//     return size;
//   }
//   int Send(char *buff, int len)
//   {
//     int size = send(this->client_fd, buff, len, 0);
//     return size;
//   }
//   void Close()
//   {
//     close(this->client_fd);
//     this->client_fd = INVALID_FD;
//     close(this->server_fd);
//     this->server_fd = INVALID_FD;
//   }
// };

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

int xioctl(int fd, int request, void *arg) {
  int r;
  do {
    r = ioctl(fd, request, arg);
  } while (r == -1 && ((errno == EINTR) || (errno == EAGAIN)));
  return r;
}


/*=======================================================================*/
/*=======================================================================*/
struct VideoDevice {
  void *start;
  size_t length;
};

typedef struct CameraParam {

} cam_para;

class V4L2 {
 public:
  int fd;
  struct VideoDevice *video_dev;
 public:
  V4L2(){}
  ~V4L2(){}

  /*以文本形式写入文件*/
  void WriteFile(std::string path, std::string str){
    std::ofstream outfile(path, std::ios::out);
    if(!outfile){
      std::cout << "open file fail: " << path << "\n";
      return;
    }
    outfile << str << std::endl;
    outfile.close();
    system("sync");
    usleep(1000*100);
  }

  int WriteImage(char *buff, int len)
  {
    char image_name[32];
    memset(image_name, 0, 32);
    sprintf(image_name, "%d×%d.jpg", width, height);
    int jpg_fd = open(image_name, O_RDWR|O_CREAT, 00700);
    if(jpg_fd == -1) {
      printf("open ipg Failed!\n ");
      return -1;
    }
    int writesize = write(jpg_fd, buff, len);
    printf("Write successfully size : %d\n", writesize);
    close(jpg_fd);
    return writesize;
  }

  bool OpenCamera()
  {
    bool res = false;
    for(int i=0; i<6; i++){
      char buff[32];
      memset(buff, '\0', 32);
      sprintf(buff, "/dev/video%d", i);
      this->fd = open(buff, O_RDWR|O_NONBLOCK);
      if(this->fd < 0){
        std::cout << "can not open " << buff << "\n";
      } else {
        std::cout << "open " << buff << "\n";
        return true;
      }
    }
    return false;
  }

  /*检查相机属性*/
  std::string GetCapability()
  {
    std::string str_fmt = "";
    str_fmt.clear();
    struct v4l2_capability cap;
    memset(&cap, 0, sizeof(struct v4l2_capability));
    int iError = xioctl(this->fd, VIDIOC_QUERYCAP, &cap);
    if(iError){
      printf("open /dev/video0: unable to query device\n");
      return "";
    }
    printf("driver:%s\n", cap.driver);
    printf("card: %s\n", cap.card);
    printf("bus_info: %s\n", cap.bus_info);
    printf("version: %d\n", cap.version);
    if(!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)){
      printf("/dev/video0 is not a video capture device\n");
      return "";
    }
    if(cap.capabilities & V4L2_CAP_STREAMING){
      printf("/dev/video supports streaming io\n");
    }
    if(cap.capabilities & V4L2_CAP_READWRITE){
      printf("/dev/video support read io\n");
    }
    struct v4l2_fmtdesc fmtc;
    memset(&fmtc, 0, sizeof(struct v4l2_fmtdesc));
    fmtc.index = 0;
    fmtc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    std::cout << "fmtc.pixelformat: ";
    while ((iError = xioctl(this->fd, VIDIOC_ENUM_FMT, &fmtc)) == 0){
      char ch1 =  fmtc.pixelformat & 0xFF;
      char ch2 = (fmtc.pixelformat>>8)  & 0xFF;
      char ch3 = (fmtc.pixelformat>>16) & 0xFF;
      char ch4 = (fmtc.pixelformat>>24) & 0xFF;
      struct v4l2_frmsizeenum frmsize;
      memset(&frmsize, 0, sizeof(struct v4l2_frmsizeenum));
      frmsize.index = 0;
      frmsize.type  = V4L2_FRMSIZE_TYPE_DISCRETE;
      frmsize.pixel_format = fmtc.pixelformat;
      while(xioctl(this->fd, VIDIOC_ENUM_FRAMESIZES, &frmsize) == 0){
        printf("%c%c%c%c(%d×%d)\n", ch1,ch2,ch3,ch4, 
                frmsize.discrete.width, frmsize.discrete.height);
        char buff[32];
        memset(buff, '\0', 32);
        sprintf(buff, "%c%c%c%c,%d*%d;", ch1,ch2,ch3,ch4, 
                      frmsize.discrete.width, frmsize.discrete.height);
        str_fmt += buff;
        frmsize.index++;
      }
      fmtc.index++;
    }
    printf("\n");

    return str_fmt;
  }

  /*设置图像帧格式*/
  bool SetFmt()
  {
    struct v4l2_format format;
    memset(&format, 0, sizeof(struct v4l2_format));
    format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    format.fmt.pix.width       = width;
    format.fmt.pix.height      = height;
    format.fmt.pix.field       = V4L2_FIELD_ANY; /*图像的扫描方式:未知或任意*/
    format.fmt.pix.pixelformat = V4L2_PIX_FMT_MJPEG;
    int iError = xioctl(this->fd, VIDIOC_S_FMT, &format);
    if(iError < 0){
      printf("unable to set format\n");
      return false;
    }
    memset(&format, 0, sizeof(struct v4l2_format));
    format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    iError = xioctl(fd, VIDIOC_G_FMT, &format);
    if(iError < 0){
      perror("get video format fail!");
      return false;
    }
    if(format.fmt.pix.width == width && format.fmt.pix.height == height &&
      format.fmt.pix.pixelformat == V4L2_PIX_FMT_MJPEG){
      printf("video: %d×%d.MJPEG\n", width,height);
    } else {
      printf("set video format fail!\n");
      return false;
    }
    return true;
  }

  bool BuffMmap()
  {
    /*申请V4L2内核缓冲区*/
    struct v4l2_requestbuffers kbuff;
    memset(&kbuff, 0, sizeof(struct v4l2_requestbuffers));
    kbuff.count  = NB_BUFFER;
    kbuff.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    kbuff.memory = V4L2_MEMORY_MMAP;
    int iError   = xioctl(this->fd, VIDIOC_REQBUFS, &kbuff);
    if(iError < 0){
      if(errno == EINVAL){
        std::cout << "this dev does not support" << "\n";
        return false;
      } else
        printf("unable to allocate buffers\n");
      return true;
    }
    /*将申请的内核缓冲帧从内核空间映射到用户空间*/
    this->video_dev = (struct VideoDevice *)
                      calloc(kbuff.count, sizeof(struct VideoDevice));
    struct v4l2_buffer sbuff;
    for(int i=0; i<(int)kbuff.count; ++i){
      memset(&sbuff, 0, sizeof(struct v4l2_buffer));
      sbuff.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
      sbuff.memory = V4L2_MEMORY_MMAP;
      sbuff.index  = i;
      iError       = xioctl(this->fd, VIDIOC_QUERYBUF, &sbuff);
      if(iError < 0){
        printf("unable to query buffer %d\n", i);
        return false;
      }
      this->video_dev[i].length = sbuff.length;
      this->video_dev[i].start = mmap(NULL, sbuff.length,
                                      PROT_READ, MAP_SHARED, this->fd,
                                      sbuff.m.offset);
      if(this->video_dev[i].start == MAP_FAILED){
        printf("unable to map buffer\n");
        return false;
      }
    }
    return true;
  }

  bool StartCaptureVideo()
  {
    /*将空闲的缓冲区加入队列*/
    struct v4l2_buffer buff;
    for(int i=0; i<NB_BUFFER; i++){
      memset(&buff, 0, sizeof(buff));
      buff.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
      buff.memory = V4L2_MEMORY_MMAP;
      buff.index = i;
      int iError = xioctl(this->fd, VIDIOC_QBUF, &buff);
      if(iError < 0){
        std::cout << "can not VIDIOC_QBUF" << "\n";
        return false;
      }
    }
    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    int iError = xioctl(this->fd, VIDIOC_STREAMON, &type);
    if(iError < 0){
      std::cout << "Do not start stream videos!" << "\n";
      return false;
    }
    return true;
  }

  int CaptureVideo(char **buff_image)
  {
    if(this->fd == INVALID_FD){
      std::cout << "dev video is not open" << "\n";
      return -1;
    }
    int size_image = -1;
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(this->fd, &fds);
    struct timeval tv;
    tv.tv_sec  = 1;
    tv.tv_usec = 1000*500;
    int res    = select(this->fd+1, &fds, NULL, NULL, &tv);
    if(res < 0){
      if(errno == EINTR){
        std::cout << "Interrupted system call" << "\n";
      }
      return 0;
    } else if(res == 0){
      std::cout << "select timeout" << "\n";
      return 0;
    } else {
      struct v4l2_buffer buff;
      buff.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
      buff.memory = V4L2_MEMORY_MMAP;
      int ret = xioctl(this->fd, VIDIOC_DQBUF, &buff);
      if(ret < 0){
        perror("VIDIOC_DQBUF fail\n");
        return false;
      }
      size_image = buff.bytesused;
      *buff_image = new char[size_image+1];
      if(*buff_image == NULL) {
        perror("malloc");
        return -1;
      }
      memcpy(*buff_image, this->video_dev[buff.index].start, buff.bytesused);
      ret = xioctl(this->fd, VIDIOC_QBUF, &buff);
      if(ret < 0){
        std::cout << "VIDIOC_QBUF fail" << "\n";
        return -1;
      }
    }
    return size_image;
  }

  bool StopCaptureVideo()
  {
    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    int iError = xioctl(this->fd, VIDIOC_STREAMOFF, &type);
    if(iError < 0){
      std::cout << "Do not stop stream videos!" << "\n";
      return false;
    }
    return true;
  }

  bool BuffUnmap()
  {
    for(int i=0; i<NB_BUFFER; i++){
      int iError = munmap(this->video_dev[i].start,
                          this->video_dev[i].length);
      if(iError < 0){
        std::cout << "munmap fail!" << "\n";
        return false;
      }
    }
    return true;
  }

  bool CloseCamera()
  {
    if(close(this->fd) < 0){
      std::cout << "close camera fail!\n" << "\n";
      return false;
    }
    this->fd = INVALID_FD;
    return true;
  }
};


static void* Capture(void* argc)
{
  pthread_t tid = pthread_self();
  printf("create pthread for camera capture, tid=%#X\n", tid);
  int *ptr_fd = (int *)argc;
  int fd = *ptr_fd;
  while(1) {

  }
}


int main(int argc, char **argv)
{
  char *param_1 = (char *)argv[1];
  int  param_2  = atoi(argv[2]);
  int  param_3  = atoi(argv[3]);
  width         = atoi(argv[4]);
  height        = atoi(argv[5]);
  printf("addrs:%s; tcp_port=%d, udp_port=%d\n", param_1, param_2, param_3);
  NetWork *tcp = new TCPNet(param_1, param_2);
  NetWork *udp = new UDPNet(param_1, param_3);

  // pthread_t tid_capture;
  // int res = pthread_create(&tid_capture, NULL, &Capture, tcp);
  // if(res == NULL){
  //   printf("create pthread for camera captrue error!!!\t");
  //   perror("create_pthread");
  //   return -1;
  // }
  // usleep(1000*200);

  V4L2 cam;
  bool res = false;
  while(1){
    if(tcp->Connect())
      printf("tcp connected!\n");

    if(udp->Connect())
      printf("udp socket!\n");

    do {
      res = cam.OpenCamera();
    } while(!res);
    printf("OpenCamera OK\n");

    std::string str = cam.GetCapability();
    std::cout << "image_format:\n" << str << std::endl;
    tcp->Send(str.c_str(), str.length());
    if(cam.SetFmt())
      printf("SetFmt OK \n");
    if(cam.BuffMmap())
      printf("BuffMmap OK\n");
    if(cam.StartCaptureVideo())
      printf("StartCaptureVideo OK\n");
    char *buff = NULL;
    int size=0, i=0, cap_count=0;

    while(1){
      size = cam.CaptureVideo(&buff);
      if(size > 0){
        printf("\niamge_size = %d\n", size);
        int k = 10;
        int index = 0;
        bool res = udp->Send(buff, size);
        if(!res)
          perror("send packet error");
        delete [] buff;
        continue;
      } else if(size < 0){
        printf("CaptureVideo fail!!!\n");
        break;
      } else if(size == 0){
        cap_count++;
        if(cap_count > 5){
          printf("CaptureVideo TimeOver!!!\n");
          break;
        }
      }
    }
    cam.StopCaptureVideo();
    cam.BuffUnmap();
    cam.CloseCamera();
    udp->Close();
  }
  // pthread_join(tid_capture, NULL);

  return 0;
}


/*
CXX=arm-linux-gnueabihf-g++
TARGET=camera

$(TARGET) : v4l2.o
	$(CXX) -o $(TARGET) v4l2.o

v4l2.o : v4l2.cpp
	$(CXX) -c -std=c++11 -lm -ljpeg -lrt v4l2.cpp

.PHONY: clean
clean:
	rm -rf $(TARGET) v4l2.o
*/