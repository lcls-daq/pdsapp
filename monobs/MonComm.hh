#ifndef PdsApp_MonShmComm_hh
#define PdsApp_MonShmComm_hh

#include "pds/service/Routine.hh"

#include "pdsdata/app/MonShmComm.hh"

namespace Pds {

  class MonEventControl {
  public:
    virtual ~MonEventControl() {}
  public:
    virtual unsigned short port() const { return MonShmComm::ServerPort; }
    virtual unsigned mask() const = 0;
    virtual bool     changed() = 0;
    virtual void     set_mask(unsigned) = 0;
  };

  class MonEventStats {
  public:
    virtual ~MonEventStats() {}
  public:
    virtual bool changed() = 0;
    virtual unsigned events() const = 0;
    virtual unsigned dmg   () const = 0;
  };

  class Task;

  class MonComm : public Routine {
  public:
    MonComm(MonEventControl& o,
            MonEventStats&   s,
            unsigned         groups);
    ~MonComm();
  public:
    void routine();
  private:
    MonEventControl& _o;
    MonEventStats&   _s;
    unsigned         _groups;
    Task*            _task;
  };
};

#endif
