#include "pdsapp/monobs/ShmClient.hh"
#include "pdsapp/monobs/Handler.hh"

#include "pds/service/Task.hh"
#include "pds/service/Timer.hh"

#include "pdsdata/xtc/Dgram.hh"
#include "pdsdata/app/XtcMonitorClient.cc"

#include "cadef.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

namespace PdsCas {
  class ShmClient::MyTimer : public Timer {
  public:
    MyTimer(unsigned   duration,
	    ShmClient& client) : 
      _duration(duration),
      _client  (client),
      _task    (new Task(TaskObject("ShmTmr")))
    {
    }
    ~MyTimer() { _task->destroy(); }
  public:
    void  expired      () { _client.update(); }
    Task* task         () { return _task; }
    unsigned duration  () const { return _duration; }
    unsigned repetitive() const { return 1; }
  private:
    unsigned   _duration;
    ShmClient& _client;
    Task*      _task;
  };

  class InitializeClient : public Pds::Routine {
  public:
    InitializeClient(ShmClient& c) : _client(c) {}
    ~InitializeClient() {}
  public:
    void routine() 
    {
      //  EPICS thread initialization
      SEVCHK ( ca_context_create(ca_enable_preemptive_callback ), 
	       "Calling ca_context_create" );
      _client.initialize(); 
      delete this; 
    }
  private:
    ShmClient& _client;
  };

  class DestroyClient : public Pds::Routine {
  public:
    void routine() {
      //  EPICS thread cleanup
      ca_context_destroy();
    }
  };
};

void usage(char* progname) {
  fprintf(stderr,"Usage: %s [-p <partitionTag>] [-h]\n", progname);
}

using namespace PdsCas;

ShmClient::ShmClient(int argc, char* argv[]) : 
  _partitionTag(0),
  _index(0)
{
  int c;
  double rate = 1.;

  while ((c = getopt(argc, argv, "?hi:p:r:")) != -1) {
    switch (c) {
    case '?':
    case 'h':
      usage(argv[0]);
      exit(0);
    case 'i':
      _index = strtoul(optarg,NULL,0);
      break;
    case 'p':
      _partitionTag = optarg;
      break;
    case 'r':
      rate = strtod(optarg,NULL);
      break;
    default:
      break;
    }
  }

  if (!_partitionTag) {
    usage(argv[0]);
    exit(0);
  }

  _timer = new MyTimer(unsigned(1000/rate),*this);
  _timer->task()->call(new InitializeClient(*this));
  _timer->start();
}

ShmClient::~ShmClient()
{
  _timer->task()->call(new DestroyClient);
  delete _timer;
  for(std::list<Handler*>::iterator it = _handlers.begin(); it != _handlers.end(); it++)
    delete (*it);
}

int ShmClient::start() { return run(_partitionTag,_index,_index); }

void ShmClient::insert(Handler* a) 
{
  _handlers.push_back(a); 
}

int ShmClient::processDgram(Pds::Dgram* dg)
{
  _seq = &dg->seq;
  iterate(&dg->xtc); 
  return 0;
}

int ShmClient::process(Pds::Xtc* xtc)
{
  if (xtc->extent < sizeof(Xtc) ||
      xtc->contains.id() >= TypeId::NumberOf)
    return 0;

  if (xtc->contains.id() == Pds::TypeId::Id_Xtc) {
    iterate(xtc);
  }
  else {
    for(HList::iterator it = _handlers.begin(); it != _handlers.end(); it++) {
      Handler* h = *it;

      if (h->info().level() == xtc->src.level() &&
          (h->info().phy  () == (uint32_t)-1 ||
           h->info().phy  () == xtc->src.phy())) {
        if (_seq->isEvent() && xtc->contains.id()==h->data_type()) {
          if (xtc->damage.value())
            h->_damaged();
          else
            h->_event(xtc->payload(),_seq->clock());
        }
        else if (_seq->service()==Pds::TransitionId::Configure &&
                 xtc->contains.id()==h->config_type()) {
          h->_configure(xtc->payload(),_seq->clock());
        }
        return 1;
      }
    }
  }
  return 1;
}

void ShmClient::initialize()
{
  for(HList::iterator it = _handlers.begin(); it != _handlers.end(); it++)
    (*it)->initialize();
}

void ShmClient::update()
{
  for(HList::iterator it = _handlers.begin(); it != _handlers.end(); it++)
    (*it)->update_pv();
}
