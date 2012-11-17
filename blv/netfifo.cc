#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>
#include <sys/ioctl.h>
#include <net/if.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <arpa/inet.h>

#include "pds/service/Task.hh"
#include "pds/service/Routine.hh"

namespace Pds {
  class ConnectRoutine : public Routine {
  public:
    ConnectRoutine( char* skt, char* fifo, bool laccept) :
      _laccept(laccept) 
    {
      strcpy(_skt,skt);
      strcpy(_fifo,fifo);
      printf("ConnectRoutine skt %s  fifo %s\n", skt, fifo);
    }
  public:
    void routine()
    {
      char* skt    = _skt;
      char* fifo   = _fifo;
      bool laccept = _laccept;

      char* addr = strtok(skt,":");
      int port = strtoul(strtok(0,":"), NULL, 0);
      const unsigned BUFF_SIZE = 4096;
      char* buff = new char[BUFF_SIZE];

      unsigned interface = 0;
      if (addr[0]<'0' || addr[0]>'9') {
        int skt = socket(AF_INET, SOCK_DGRAM, 0);
        if (skt<0) {
          perror("Failed to open socket\n");
          exit(1);
        }
        ifreq ifr;
        strcpy( ifr.ifr_name, addr);
        if (ioctl( skt, SIOCGIFADDR, (char*)&ifr)==0)
          interface = ntohl( *(unsigned*)&(ifr.ifr_addr.sa_data[2]) );
        else {
          printf("Cannot get IP address for network interface %s.\n",addr);
        }
        printf("Using interface %s (%d.%d.%d.%d)\n",
               addr,
               (interface>>24)&0xff,
               (interface>>16)&0xff,
               (interface>> 8)&0xff,
               (interface>> 0)&0xff);
        close(skt);
      }
      else {
        in_addr inp;
        if (inet_aton(addr, &inp))
          interface = ntohl(inp.s_addr);
      }

      sockaddr_in sa;
      sa.sin_family = AF_INET;
      sa.sin_addr.s_addr = htonl(interface);
      sa.sin_port        = htons(port);

      if (laccept) {
        int listen_fd = ::socket(AF_INET, SOCK_STREAM, 0);
        int32_t opt=1;
        if (setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))<0) {
          perror("Reuse listen addr failed");
          return;
        }
        ::bind(listen_fd,(sockaddr*)&sa,sizeof(sa));

        while(1) {
          printf("Listening [%x.%d].\n",interface,port);
          if (::listen(listen_fd,25)<0)
            perror("Listen failed\n");
          else {
            printf("Connecting [%x.%d].\n",interface,port);
            sockaddr_in remote;
            socklen_t   remote_len = sizeof(remote);
            int skt_fd = ::accept(listen_fd,(sockaddr*)&remote,&remote_len);
            if (skt_fd<0)
              perror("Accept failed\n");
            else {
              printf("Connected [%x.%d].  Opening FIFO [%s]...\n",
                     interface,port,fifo);
              // now open the fifo for output
              int fifo_fd  = ::open(fifo,O_WRONLY);
              printf("FIFO Open for writing [%s]\n",fifo);
              int len;
              while((len = ::read(skt_fd, buff, BUFF_SIZE))>=0) {
                if (len)
                  ::write(fifo_fd, buff, len);
              }
              printf("Closing output FIFO\n");
              ::close(fifo_fd);
              ::close(skt_fd);
            }
          }
        }
      }
      else {
        while(1) {
          int skt_fd = ::socket(AF_INET, SOCK_STREAM, 0);
          if (skt_fd == -1) {
            perror("Socket failed");
            return;
          }
          printf("Connecting [%x.%d].\n",interface,port);
          if (::connect(skt_fd, (sockaddr*)&sa, sizeof(sa))) {
            perror("Error connecting");
            printf("Retry in 5 seconds...\n");
            timespec ts; ts.tv_sec = 5; ts.tv_nsec = 0;
            nanosleep(&ts,0);
          }
          else {
            printf("Connected [%x.%d].  Opening FIFO for reading... [%s]\n",
                   interface,port,fifo);
            // now open the fifo for input
            int fifo_fd  = ::open(fifo,O_RDONLY);
            printf("FIFO open for reading [%s].\n",fifo);
            int len;
            while((len = ::read(fifo_fd, buff, BUFF_SIZE))>=0) {
              if (len)
                ::write(skt_fd, buff, len);
            }
            printf("Closing input FIFO\n");
            ::close(fifo_fd);
          }
          ::close(skt_fd);
        }
      }
      delete buff;
      delete this;
    }
  private:
    char  _skt [128];
    char  _fifo[128];
    bool  _laccept;
  };
};

using namespace Pds;


void usage(const char* p) {
  printf("Usage: %s -c <input,output> [-c <input,output>]\n",p);
  printf("\t <input/output> is <host>:<port> or <FIFO name>\n");
}

int main(int argc, char** argv) {

  extern char* optarg;
  int c;
  unsigned nconnect=0;

  while ( (c=getopt( argc, argv, "c:")) != EOF ) {
    switch(c) {
    case 'c':
      { char* input  = strtok(optarg,",");
        char* output = strtok(NULL,",");
        ConnectRoutine* routine;
        if (strchr(input,':')==NULL) // input is a FIFO
          routine = new ConnectRoutine(output,input,false);
        else
          routine = new ConnectRoutine(input,output,true);

        char taskname[32];
        sprintf(taskname,"nfifo%d",nconnect++);
        Task* task = new Task(TaskObject(taskname));
        task->call(routine);
        break; }
    default:
      usage(argv[0]);
      exit(1);
    }
  }

  Task* task = new Task(Task::MakeThisATask);
  task->mainLoop();

  return 0;
}
