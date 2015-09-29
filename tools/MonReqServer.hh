#ifndef Pds_MonReqServer_hh
#define Pds_MonReqServer_hh

#include "pds/utility/Appliance.hh"
#include "pds/service/Routine.hh"
#include "pds/service/Task.hh"

namespace Pds {
  
  class MonReqServer : public Appliance, public Routine {
  public:
    //MonReqServer();
    MonReqServer(unsigned int& k);
  public:
    ~MonReqServer();
  public:
    Transition* transitions(Transition*);
    InDatagram* events     (InDatagram*);

    void routine();
  private:
    Task* _task;

  private:
    char* tcp_ip;
    char* tcp_port;

  private:
    int tcp_socket_info;

  private: 
    int number_of_requests_fullfilled;
    int number_of_requests_received;

  private:
  int id;


  };
};

#endif
