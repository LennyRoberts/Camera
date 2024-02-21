#include "../inc/network.hpp"
#include "../inc/v4l2cam.hpp"

#include <pthread.h>

Camera *cam;

bool flag_recv = false;
void* PthCamCapture(void* argc)
{
  pthread_t tid = pthread_self();
  printf("create pthread for camera capture, tid=%#X\n", tid);
  NetWork *udp = (NetWork*)argc;
  char buff[32];
  while(1) {
    memset(buff, '\0', 32);
    int size = udp->Recv(buff, sizeof(buff));
    if(size >0 && !strncmp(buff, "recv_cap", 7)){
      printf("recv server reply!\n");
      flag_recv = true;
    }
  }
}

std::string ExeTCPCmd(std::string data, int size)
{
  neb::CJsonObject tcp_json(data);
  int cmd = 0;
  tcp_json.Get("cmd", cmd);
  int len = 0;
  neb::CJsonObject json_res;
  std::string str_res;
  switch(cmd){
    case 0x0103: {
      std::string str_camcap;
      uint16 cam_num = cam->OpenCamera(str_camcap);
      json_res.Add("cmd", cmd);
      json_res.Add("cam_num", cam_num);
      json_res.Add("para", str_camcap);
      str_res = json_res.ToString();
      break;
    }
    case 0x0104: {
      int cam_index = 0, fmt_index = 0;
      tcp_json.Get("cam_i", cam_index);
      tcp_json.Get("fmt_i", fmt_index);
      json_res.Add("cmd", cmd);
      bool res = false;
      res = cam->InitCamCap(cam_index, fmt_index);
      if(res)
        res = cam->SetCapture(cam_index);
      json_res.Add("res", res);
      str_res = json_res.ToString();
      break;
    }
    case 0x0105:{
      int cam_index = 0, fmt_index = 0;
      tcp_json.Get("cam_i", cam_index);
      bool res = cam->ClearCapture(cam_index);
      res = cam->CloseCamera(cam_index);
      json_res.Add("cmd", cmd);
      json_res.Add("res", res);
      str_res = json_res.ToString();
      break;
    }
  }
  return str_res;
}

void* PthTCPRecv(void *arg)
{
  TCPNet *tcp = (TCPNet*)arg;
  int size = 0;
  char buff[4096];
  while(1){
    if(tcp->net_stu == NetStatu::StatuLink){
      memset(buff, 0, 4096);
      size = tcp->Recv(buff, sizeof(buff));
      if(size > 0){
        printf("recv data: %s\n", buff);
        std::string str_res = ExeTCPCmd(buff, size);
        if(!str_res.empty()){
          if(tcp->net_stu == NetStatu::StatuLink){
            int len = tcp->Send(str_res.c_str(), str_res.size());
            if(len > 0)
              printf("\nsend to server:\n %s\n", str_res.c_str());
          }
        }
      }
    }
  }
}

void* UDPNetCapture(void *arg)
{
  UDPNet *udp_net = (UDPNet*)arg;
  while(1){
    if(cam->num_cam == 0){
      usleep(1000*500);
      continue;
    }
    std::vector<V4L2>::iterator it;
    for(it=cam->vec_cam_v4l2.begin();
        it!=cam->vec_cam_v4l2.end(); ++it){
      if((*it).flag_cap &&
         (*it).cam_stu == CameraStu::is_OK){
        char *buff = NULL;
        int size = (*it).CaptureVideo(&buff);
        printf("capture %d byte picture\n", size);
        if(size > 0){
          // printf("capture %s img size = %d\n", (*it).cam_para->card.c_str(), size);
          if(udp_net->isNetStatus(NetStatu::StatuLink))
            udp_net->SendImg(buff, size, (*it).cur_cam, (*it).cur_fmt);
          if(buff != NULL)
            delete [] buff;
        }
      }
    }
  }
}

int main(int argc, char **argv)
{
  char *param_1 = (char *)argv[1];
  int  param_2  = atoi(argv[2]);
  int  param_3  = atoi(argv[3]);
  cam = new Camera();
  TCPNet *tcp = new TCPNet();
  tcp->en_bit = true;
  UDPNet *udp = new UDPNet();
  udp->en_bit = true;
  if(udp->Connect(param_1, param_3))
    printf("UDP connect successful\n");


  pthread_t tid_tcp_recv;
  int res = pthread_create(&tid_tcp_recv, NULL, PthTCPRecv, tcp);
  if(res != 0){
    perror("pthread create for rcp recv!");
    return 0;
  }

  pthread_t tid_udp;
  res = pthread_create(&tid_udp, NULL, UDPNetCapture, udp);
  if(res != 0){
    perror("pthread create for udp net!");
    return 0;
  }

  while(1){
    if(tcp->en_bit){
      if(tcp->net_stu == NetStatu::StatuRunFail){
        if(tcp->Connect(param_1, param_2)){
          printf("TCP connect to %s:%d\n", tcp->GetAddrs().c_str(), tcp->GetPort());
        }
      }
    } else {
      if(tcp->net_stu == NetStatu::StatuLink){
        tcp->Close();
      }
    }
    sleep(1);
  }
  // while(1){
  //   char buff[4096];
  //   memset(buff, 0, 4096);
  //   int size = tcp->Recv(buff, sizeof(buff));
  //   if(size > 0){
  //     printf("recv data is:\n%s\n", buff);
  //     memset(buff, 0, 4096);
  //     sprintf(buff, "Hello Server, I have a messge form you\n");
  //     size = tcp->Send(buff, sizeof(buff)); 
  //     if(size > 0)
  //       printf("send data to server: %s\n", buff);
  //   }
  // }
  // for(int i=0; i< 1000; ++i){
  //   char buff[32];
  //   memset(buff, 0, 32);
  //   sprintf(buff, "%02dth pack form client\n", i);
  //   int size = tcp->Send(buff, sizeof(buff));
  //   if(size > 0){
  //     printf("send successfully: %s\n", buff);
  //     usleep(1000*100);
  //   } else {
  //     printf("send %d packet fail!!!\n");
  //   }
  // }

  // pthread_t tid_capture;
  // int res = pthread_create(&tid_capture, NULL, &Capture, udp);
  // if(res != 0){
  //   printf("create pthread for camera captrue error!!!\t");
  //   perror("create_pthread");
  //   return -1;
  // }
  // usleep(1000*200);

  // Camera cam;
  // std::string str_camcap;
  // cam.OpenCamera(str_camcap);
  // std::cout << str_camcap << std::endl;
  // for(int i=0; i< 2; ++i){
  //   if(flag_recv)
  //     break;
  //   udp->Send(str_camcap.c_str(), str_camcap.length());
  //   flag_recv = false;
  // }
  // cam.CloseAll();
  // pthread_join(tid_capture, NULL);
  // pthread_cancel(tid_capture);

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