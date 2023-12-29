#include "../inc/network.hpp"
#include "../inc/v4l2cam.hpp"

#include <pthread.h>

// static void* Capture(void* argc)
// {
//   pthread_t tid = pthread_self();
//   printf("create pthread for camera capture, tid=%#X\n", tid);
//   int *ptr_fd = (int *)argc;
//   int fd = *ptr_fd;
//   while(1) {

//   }
// }

int main(int argc, char **argv)
{
  Camera cam;
  std::string str_camcap;
  cam.OpenCamera(str_camcap);
  std::cout << str_camcap << std::endl;
  cam.CloseAll();


  // char *param_1 = (char *)argv[1];
  // int  param_2  = atoi(argv[2]);
  // int  param_3  = atoi(argv[3]);
  // width         = atoi(argv[4]);
  // height        = atoi(argv[5]);
  // printf("addrs:%s; tcp_port=%d, udp_port=%d\n", param_1, param_2, param_3);
  // NetWork *tcp = new TCPNet(param_1, param_2);
  // NetWork *udp = new UDPNet(param_1, param_3);

  // pthread_t tid_capture;
  // int res = pthread_create(&tid_capture, NULL, &Capture, tcp);
  // if(res == NULL){
  //   printf("create pthread for camera captrue error!!!\t");
  //   perror("create_pthread");
  //   return -1;
  // }
  // usleep(1000*200);
  // std::vector<V4L2> vec_cam;
  // for(int i=0; i<6; ++i){
  //   V4L2 cam;
  //   int res = cam.OpenCam(i);
  //   if(res > 0){
  //     std::vector<cam_fmt>::iterator it;
  //     for(it=cam.cam_para->cfmt.begin(); it!=cam.cam_para->cfmt.end(); ++it){
  //       std::string str_format = cam.PixeFormatToString((*it).pixelformat);
  //       printf("index=%02d; format=%s(%d); width=%d; height=%d\n",
  //              (*it).index, str_format.c_str(), (*it).pixelformat, (*it).width, (*it).height);
  //     }
  //     vec_cam.push_back(cam);
  //     std::cout << "push cam to vector\n";
  //   } else {
  //     std::cout << "open camera fail " << i << std::endl;
  //   }
  // }

  

  // bool res = false;
  // while(1){
  //   if(tcp->Connect())
  //     printf("tcp connected!\n");

  //   if(udp->Connect())
  //     printf("udp socket!\n");

  //   do {
  //     res = cam.OpenCamera();
  //   } while(!res);
  //   printf("OpenCamera OK\n");

  //   std::string str = cam.GetCapability();
  //   std::cout << "image_format:\n" << str << std::endl;
  //   tcp->Send(str.c_str(), str.length());
  //   if(cam.SetFmt())
  //     printf("SetFmt OK \n");
  //   if(cam.BuffMmap())
  //     printf("BuffMmap OK\n");
  //   if(cam.StartCaptureVideo())
  //     printf("StartCaptureVideo OK\n");
  //   char *buff = NULL;
  //   int size=0, i=0, cap_count=0;

  //   while(1){
  //     size = cam.CaptureVideo(&buff);
  //     if(size > 0){
  //       printf("\niamge_size = %d\n", size);
  //       int k = 10;
  //       int index = 0;
  //       bool res = udp->Send(buff, size);
  //       if(!res)
  //         perror("send packet error");
  //       delete [] buff;
  //       continue;
  //     } else if(size < 0){
  //       printf("CaptureVideo fail!!!\n");
  //       break;
  //     } else if(size == 0){
  //       cap_count++;
  //       if(cap_count > 5){
  //         printf("CaptureVideo TimeOver!!!\n");
  //         break;
  //       }
  //     }
  //   }
  //   cam.StopCaptureVideo();
  //   cam.BuffUnmap();
  //   cam.CloseCamera();
  //   udp->Close();
  // }
  // pthread_join(tid_capture, NULL);

  return 0;
}