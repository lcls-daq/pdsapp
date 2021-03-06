#ifndef PdsCas_ShmClient_hh
#define PdsCas_ShmClient_hh

#include "pdsdata/app/XtcMonitorClient.hh"
#include "pdsdata/xtc/XtcIterator.hh"

#include <list>

namespace Pds { class Sequence; }

namespace PdsCas {
  class Handler;
  class EvtHandler;
  class ShmClient : public Pds::XtcMonitorClient,
		    private Pds::XtcIterator {
  public:
    ShmClient();
    ~ShmClient();
  public:
    bool arg  (char, const char*);
    bool valid() const;
    static const char* opts   ();
    static const char* options();
  public:
    void insert(Handler*);
    void insert(EvtHandler*);
    int  start ();
  public:
    int processDgram(Pds::Dgram*);
    int process(Pds::Xtc* xtc);
  public:
    void initialize();
    void update();
  private:
    typedef std::list<Handler*> HList;
    HList       _handlers;
    typedef std::list<EvtHandler*> EList;
    EList       _ehandlers;
    const char* _partitionTag;
    unsigned    _index;
    int         _evindex;
    double      _rate;
    class MyTimer;
    MyTimer* _timer;
    const Pds::Sequence* _seq;
  };
};

#endif
