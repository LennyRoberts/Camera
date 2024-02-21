#include <linux/videodev2.h>
#include <sys/select.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <string>
#include <map>
#include <vector>

#include "../lib/cJsonObject/CJsonObject.hpp"

#define CAMERA_NUM         6
#define INVALID_FD        -1
#define NB_BUFFER          4

enum CameraStu {
  is_open    = 1,
  is_notReady = 2,
  is_OK       = 3,
  is_close    = 4,
};

int width = 0;
int height= 0;

int xioctl(int fd, int request, void *arg) {
  int r;
  do {
    r = ioctl(fd, request, arg);
  } while (r == -1 && ((errno == EINTR) || (errno == EAGAIN)));
  return r;
}

struct VideoDevice {
  void *start;
  size_t length;
};

class CamFmt {
 public:
  unsigned int fmt_index;
  unsigned int width;
  unsigned int height;
  unsigned int pixelformat;
};

class CameraParam {
 public:
  std::string    driver;    /*@driver:   驱动程序模块的名称（例如“bttv”*/
  std::string    card;      /*@card:     卡的名称（例如“Hauppauge WinTV”）*/
  std::string    bus_info;  /*@bus_info: 总线的名称（例如“PCI:”+PCI_name（PCI_dev））*/
  std::string    version;   /*@version:  KERNEL_version*/
  unsigned short num_fmt;   /*@num_fmt:  相机支持格式数量*/
  unsigned short cam_index; /*@index:    相机的索引序号, -1表示没有相机**/
  std::vector<CamFmt> cfmt; /*@cfmt:     format and size*/
};

class V4L2 {
 public:
  int         fd;
  bool        flag_cap;
  CameraStu   cam_stu;
  CameraParam *cam_para;
  uint16      cur_cam;
  uint16      cur_fmt;   
  struct VideoDevice *video_dev;
 public:
  V4L2()
  {
    this->cam_stu   = CameraStu::is_close;
    this->flag_cap  = false;
    this->cam_para  = NULL;
    this->video_dev = NULL;
    this->cur_cam   = 0;
    this->cur_fmt   = 0;
  }
  ~V4L2()
  {
    if(this->cam_para != NULL){
      free(this->cam_para);
      delete this->cam_para;
      this->cam_para  = NULL;
    }
    if(this->video_dev != NULL){
      free(this->video_dev);
      this->video_dev = NULL;
    }
    this->cam_stu = CameraStu::is_close;
  }

  void Clear()
  {
    if(this->cam_para != NULL){
      // free(this->cam_para);
      delete this->cam_para;
      this->cam_para  = NULL;
    }
    if(this->video_dev != NULL){
      free(this->video_dev);
      this->video_dev = NULL;
    }
    this->cam_stu  = CameraStu::is_close;
    this->flag_cap = false;
  }

  std::string PixeFormatToString(unsigned int pixelformat)
  {
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

  int OpenCam(int cam_index)
  {
    int res = INVALID_FD;
    char name[16]; 
    memset(name, '\0', 16);
    sprintf(name, "/dev/video%d", cam_index);
    res = open(name, O_RDWR|O_NONBLOCK);
    if(res > 0){
      this->fd = res;
      this->cam_stu  = CameraStu::is_open;
      this->cam_para = new CameraParam();
      if(this->cam_para == NULL){
        perror("malloc camera_param");
        return false;
      }
      this->cam_para->cam_index = cam_index;
      GetCapability(cam_index);
    } else {
      this->cam_stu = CameraStu::is_close;
    }
    return res;
  }

  /*检查相机属性*/
  bool GetCapability(int cam_index)
  {
    struct v4l2_capability cap;
    memset(&cap, 0, sizeof(struct v4l2_capability));
    int iError = xioctl(this->fd, VIDIOC_QUERYCAP, &cap);
    if(iError){
      printf("open /dev/video: unable to query device\n");
      return false;
    }

    if(!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)){
      printf("/dev/video0 is not a video capture device\n");
      return false;
    }
    if(cap.capabilities & V4L2_CAP_STREAMING){
      printf("/dev/video supports streaming io\n");
    }
    if(cap.capabilities & V4L2_CAP_READWRITE){
      printf("/dev/video support read io\n");
    }
    // this->cam_para = (camera_param*)malloc(sizeof(camera_param));
    
    char buff[64];
    memcpy(buff, cap.driver, sizeof(cap.driver));
    this->cam_para->driver = buff; 
    printf("driver:%s\n", this->cam_para->driver.c_str());

    this->cam_para->card     = (char*)cap.card;
    printf("card: %s\n", this->cam_para->card.c_str());

    this->cam_para->bus_info = (char*)cap.bus_info;
    printf("bus_info: %s\n", this->cam_para->bus_info.c_str());

    this->cam_para->version  = std::to_string(cap.version);
    printf("version: %s\n", this->cam_para->version.c_str());

    struct v4l2_fmtdesc fmtc;
    memset(&fmtc, 0, sizeof(struct v4l2_fmtdesc));
    fmtc.index = 0;
    fmtc.type  = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    int i = 0;
    while ((iError = xioctl(this->fd, VIDIOC_ENUM_FMT, &fmtc)) == 0){
      struct v4l2_frmsizeenum frmsize;
      memset(&frmsize, 0, sizeof(struct v4l2_frmsizeenum));
      frmsize.index = 0;
      frmsize.type  = V4L2_FRMSIZE_TYPE_DISCRETE;
      frmsize.pixel_format = fmtc.pixelformat;
      while(xioctl(this->fd, VIDIOC_ENUM_FRAMESIZES, &frmsize) == 0){
        CamFmt fmt;
        fmt.fmt_index  = i;
        fmt.width  = frmsize.discrete.width;
        fmt.height = frmsize.discrete.height;
        fmt.pixelformat = fmtc.pixelformat;
        this->cam_para->cfmt.push_back(fmt);
        this->cam_para->num_fmt++;
        frmsize.index++;
        i++;
      }
      fmtc.index++;
    }
    return true;
  }

  /*设置图像帧格式*/
  bool SetFmt(unsigned short index)
  {
    if(index > (this->cam_para->num_fmt-1)){
      printf("!!! fmt_index overflow\n");
      return false;
    }
    CamFmt *fmt = (CamFmt*)&this->cam_para->cfmt[index];
    if(index != fmt->fmt_index){
      std::vector<CamFmt>::iterator it;
      for(it=this->cam_para->cfmt.begin(); 
          it!=this->cam_para->cfmt.end(); ++it){
        if(index == (*it).fmt_index){
          fmt = (CamFmt*)&(*it); 
          break;
        }
      }
    }
    struct v4l2_format format;
    memset(&format, 0, sizeof(struct v4l2_format));
    format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    format.fmt.pix.width       = fmt->width;
    format.fmt.pix.height      = fmt->height;
    format.fmt.pix.field       = V4L2_FIELD_ANY; /*图像的扫描方式:未知或任意*/
    format.fmt.pix.pixelformat = fmt->pixelformat;
    int iError = xioctl(this->fd, VIDIOC_S_FMT, &format);
    if(iError < 0){
      perror("unable to set format");
      return false;
    }
    memset(&format, 0, sizeof(struct v4l2_format));
    format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    iError = xioctl(fd, VIDIOC_G_FMT, &format);
    if(iError < 0){
      perror("VIDIOC_G_FMT error");
      return false;
    }
    if(format.fmt.pix.width == fmt->width && 
       format.fmt.pix.height == fmt->height &&
       format.fmt.pix.pixelformat == fmt->pixelformat){
      printf("video: %d×%d.%s\n", fmt->width,fmt->height, 
             this->PixeFormatToString(fmt->pixelformat).c_str());
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
      perror("allocate error");
      if(errno == EINVAL){
        printf("this dev does not support\n");
        return false;
      }
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
        perror("VIDIOC_QUERYBUF error");
        return false;
      }
      this->video_dev[i].length = sbuff.length;
      this->video_dev[i].start = mmap(NULL, sbuff.length,
                                      PROT_READ, MAP_SHARED, this->fd,
                                      sbuff.m.offset);
      if(this->video_dev[i].start == MAP_FAILED){
        perror("buffer mmap error");
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
        perror("VIDIOC_QBUF error");
        return false;
      }
    }
    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    int iError = xioctl(this->fd, VIDIOC_STREAMON, &type);
    if(iError < 0){
      perror("VIDIOC_STREAMON error");
      return false;
    }
    this->cam_stu = CameraStu::is_OK;
    return true;
  }

  int CaptureVideo(char **buff_image)
  {
    if(this->fd == INVALID_FD){
      std::cout << "dev video is not open" << "\n";
      return -1;
    }
    if(this->cam_stu != CameraStu::is_OK)
      return -1;
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
      return -1;
    } else if(res == 0){
      std::cout << "select timeout" << "\n";
      return -1;
    } else {
      struct v4l2_buffer buff;
      buff.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
      buff.memory = V4L2_MEMORY_MMAP;
      int ret = xioctl(this->fd, VIDIOC_DQBUF, &buff);
      if(ret < 0){
        perror("VIDIOC_DQBUF fail\n");
        return -1;
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
    this->cam_stu = CameraStu::is_notReady;
    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    int iError = xioctl(this->fd, VIDIOC_STREAMOFF, &type);
    if(iError < 0){
      perror("VIDIOC_STREAMOFF error");
      return false;
    }
    return true;
  }

  bool BuffUnmap()
  {
    this->cam_stu = CameraStu::is_notReady;
    for(int i=0; i<NB_BUFFER; i++){
      int iError = munmap(this->video_dev[i].start,
                          this->video_dev[i].length);
      if(iError < 0){
        perror("buffer unmap error");
        return false;
      }
    }
    if(this->video_dev != NULL){
      free(this->video_dev);
      this->video_dev = NULL;
    }
    return true;
  }

  bool CloseCam()
  {
    this->cam_stu = CameraStu::is_close;
    if(close(this->fd) < 0){
      std::cout << "close camera fail!\n" << "\n";
      return false;
    }
    this->Clear();
    this->fd = INVALID_FD;
    return true;
  }
};

class Camera {
 public:
  unsigned short num_cam;
  std::vector<V4L2> vec_cam_v4l2;
 public:
  Camera()
  {
    this->num_cam = 0;
    this->vec_cam_v4l2.reserve(CAMERA_NUM);
    for(int i=0; i<CAMERA_NUM; ++i){
      V4L2 cam_v4l2;
      this->vec_cam_v4l2.push_back(cam_v4l2);
    }
  }
  ~Camera(){}

  /**
   * @brief 打开所有相机
   * @param str_camcap 所有相机的属性
   * @return 打开的相机个数 */
  unsigned short OpenCamera(std::string &str_camcap)
  {
    this->CloseAll();
    neb::CJsonObject json_cam;
    for(int index=0; index<CAMERA_NUM; ++index){
      neb::CJsonObject json_v4l2;
      V4L2 *cam_v4l2 = (V4L2*)&this->vec_cam_v4l2[index];
      int res = -1;
      // if(cam_v4l2->cam_stu == CameraStu::is_OK){

      // } else {
        res  = cam_v4l2->OpenCam(index);
      // }
      if(res > 0){
        json_v4l2.Add("cam_index", cam_v4l2->cam_para->cam_index);
        json_v4l2.Add("driver", cam_v4l2->cam_para->driver);
        json_v4l2.Add("card", cam_v4l2->cam_para->card);
        json_v4l2.Add("bus_info", cam_v4l2->cam_para->bus_info);
        json_v4l2.Add("version", cam_v4l2->cam_para->version);
        json_v4l2.Add("num_fmt", cam_v4l2->cam_para->num_fmt);
        std::vector<CamFmt>::iterator it;
        for(it=cam_v4l2->cam_para->cfmt.begin(); 
            it!=cam_v4l2->cam_para->cfmt.end(); ++it){
          neb::CJsonObject json_fmt;
          json_fmt.Add("fmt_index", (*it).fmt_index);
          std::string str_format = 
                      cam_v4l2->PixeFormatToString((*it).pixelformat);
          json_fmt.Add("format", str_format);
          json_fmt.Add("width", (*it).width);
          json_fmt.Add("height", (*it).height);
          std::string str_fmt = "fmt" + std::to_string((*it).fmt_index);
          json_v4l2.Add(str_fmt, json_fmt);
        }
        // cam_v4l2->cam_para->num_fmt = fmt_step;
        // json_v4l2.Add("num_fmt", cam_v4l2->cam_para->num_fmt);
        std::string str_cam = "cam" + std::to_string(index);
        json_cam.Add(str_cam, json_v4l2);
        this->num_cam++;
        std::cout << "push cam to vector\n";
      } else {
        std::cout << "open camera fail " << index << std::endl;
      }
    }
    str_camcap = json_cam.ToString();
    return this->num_cam;
  }

  /**
   * @brief 相机初始化设置，设置属性/设置缓存映射/开启数据采集
   *        在相机正在工作时,会先停止采集,清除缓存映射,然后再重新初始化
   * @param cam_index 相机的索引
   * @param cap_index 该相机属性设置的索引
   * @return true=成功; false=失败 */
  bool InitCamCap(unsigned short cam_index, unsigned short fmt_index)
  {
    bool res = false;
    if(cam_index > CAMERA_NUM || cam_index < 0){
      printf("invalid camera index!!\n");
      return res;
    }
    V4L2 *cam_v4l2 = (V4L2*)&this->vec_cam_v4l2[cam_index];
    if(cam_v4l2->cam_stu == CameraStu::is_close){
      printf("this camera invalid!!\n");
      return false;
    }

    if(cam_v4l2->cam_stu == CameraStu::is_OK){
      cam_v4l2->StopCaptureVideo();
      cam_v4l2->BuffUnmap();
    }

    res = cam_v4l2->SetFmt(fmt_index);
    if(!res){
      return res;
    }
    res = cam_v4l2->BuffMmap();
    if(!res){
      return res;
    }
    res = cam_v4l2->StartCaptureVideo();
    if(!res){
      return res;
    }
    cam_v4l2->cur_cam = cam_index;
    cam_v4l2->cur_fmt = fmt_index;
    return res;
  }

  bool SetCapture(unsigned short cam_index)
  {
    if(cam_index > CAMERA_NUM || cam_index < 0){
      printf("invalid camera index!!\n");
      return false;
    }
    V4L2 *cam_v4l2 = (V4L2*)&this->vec_cam_v4l2[cam_index];
    if(cam_v4l2->cam_stu == CameraStu::is_close){
      printf("this camera invalid!!\n");
      return false;
    }
    cam_v4l2->flag_cap = true;
    return true;
  }
  
  bool ClearCapture(unsigned short cam_index)
  {
    if(cam_index > CAMERA_NUM || cam_index < 0){
      printf("invalid camera index!!\n");
      return false;
    }
    V4L2 *cam_v4l2 = (V4L2*)&this->vec_cam_v4l2[cam_index];
    if(cam_v4l2->cam_stu == CameraStu::is_close){
      printf("this camera invalid!!\n");
      return false;
    }
    cam_v4l2->flag_cap = false;
    return true;
  }

  /**
   * @brief 采集相片
   * @param cam_index 指定相机源
   * @param buff 传入一个二级指针用于开辟空间并存放采集数据,
   *             当采集成功时必须要在外部释放掉内部开辟的内存空间,
   *             否则将造成持续的内存泄露
   * @return 采集失败=-1，成功=采集到的字节数 */
  int Capture(unsigned short cam_index, char **buff)
  {
    if(cam_index > CAMERA_NUM || cam_index < 0){
      printf("invalid camera index!!\n");
      return -1;
    }
    V4L2 *cam_v4l2 = (V4L2*)&this->vec_cam_v4l2[cam_index];
    if(cam_v4l2->cam_stu == CameraStu::is_close){
      printf("this camera invalid!!\n");
      return -1;
    }
    int size = cam_v4l2->CaptureVideo(buff);
    return size;
  }

  /**
   * @brief 关闭相机
   * @param cam_index 指定相机源
   * @return true=成功; false=失败 */
  bool CloseCamera(unsigned short cam_index)
  {
    bool res = false;
    if(cam_index > 6 || cam_index < 0){
      printf("invalid camera index!!\n");
      return res;
    }
    V4L2 *cam_v4l2 = (V4L2*)&this->vec_cam_v4l2[cam_index];
    if(cam_v4l2->cam_stu == CameraStu::is_close){
      printf("this camera(/dev/video%d) is not opened!!\n", cam_index);
      return false;
    }
    res = cam_v4l2->StopCaptureVideo();
    if(!res){
      return res;
    }
    res = cam_v4l2->BuffUnmap();
    if(!res){
      return res;
    }
    res = cam_v4l2->CloseCam();
    if(!res){
      return res;
    }
    this->num_cam--;
    return res;
  }

  void CloseAll()
  {
    for(int index=0; index< CAMERA_NUM; ++index){
      CloseCamera(index);
    }
  }
};
