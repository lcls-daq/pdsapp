#include "pdsapp/monobs/MonComm.hh"

#include "pds/service/Task.hh"
#include "pds/service/Ins.hh"
#include "pds/service/Sockaddr.hh"

#include <unistd.h>
#include <stdlib.h>
#include <poll.h>

using namespace Pds;

MonComm::MonComm(MonEventControl& o,
                 MonEventStats&   s,
                 unsigned         groups) :
  _o(o), _s(s), _groups(groups), _task(new Task(TaskObject("comm")))
{
  _task->call(this);
}

MonComm::~MonComm()
{
  _task->destroy();
}

void MonComm::routine() 
{
  MonShmComm::Get get;
  gethostname(get.hostname,sizeof(get.hostname));

  unsigned short insp = MonShmComm::ServerPort;
  if (strncmp(get.hostname,"daq",3)!=0)
    insp = _o.port();
  Ins ins(insp);
  printf("Listening on port %d [%s]\n",insp,get.hostname);

  int _socket;
  if ((_socket = ::socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    perror("Comm failed to open socket");
    exit(1);
  }

  int parm = 16*1024*1024;
  if(setsockopt(_socket, SOL_SOCKET, SO_SNDBUF, (char*)&parm, sizeof(parm)) == -1) {
    perror("Comm failed to set sndbuf");
    exit(1);
  }

  if(setsockopt(_socket, SOL_SOCKET, SO_RCVBUF, (char*)&parm, sizeof(parm)) == -1) {
    perror("Comm failed to set rcvbuf");
    exit(1);
  }

  int optval=1;
  if (setsockopt(_socket, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0) {
    perror("Comm failed to set reuseaddr");
    exit(1);
  }

  Sockaddr sa(ins);
  if (::bind(_socket, sa.name(), sa.sizeofName()) < 0) {
    perror("Comm failed to bind to port");
    exit(1);
  }

  while(1) {
    printf("Comm listening\n");
    if (::listen(_socket,10)<0)
      perror("Comm listen failed");
    else {
      Sockaddr name;
      unsigned length = name.sizeofName();
      int s = ::accept(_socket,name.name(), &length);
      if (s<0) {
        perror("Comm accept failed");
      }
      else {

        printf("Comm accept connection from %x.%d\n",
               name.get().address(),name.get().portId());

        int    nfds=1;
        pollfd pfd[1];
        pfd[0].fd = s;
        pfd[0].events = POLLIN | POLLERR;
        int timeout = 1000;

        bool changed = true;
        while(1) {
          changed |= _o.changed();
          changed |= _s.changed();
          changed = true;
          if (changed) {
            get.groups = _groups;
            get.mask   = _o.mask  ();
            get.events = _s.events();
            get.dmg    = _s.dmg   ();
            ::write(s,&get,sizeof(get));
            changed = false;
          }

          pfd[0].revents = 0;
          if (::poll(pfd,nfds,timeout)>0) {
            if (pfd[0].revents & (POLLIN|POLLERR)) {
              MonShmComm::Set set;
              int r = ::read(s, &set, sizeof(set));
              if (r<0) {
                perror("MonShmComm socket read error");
                break;
              }
              else if (unsigned(r)<sizeof(set)) {
                printf("MonShmComm socket closed [%d/%zu]\n",r,sizeof(set));
                break;
              }
              if (strcmp(set.hostname,get.hostname)!=0) {
                printf("MonShmComm socket name invalid [%s/%s]\n",
                       set.hostname,get.hostname);
                break;
              }
              _o.set_mask(set.mask);
            }
          }
        }
        printf("Comm closed connection\n");
        ::close(s);
      }
    }
  }
}
