#ifndef Pds_MonMAIN_HH
#define Pds_MonMAIN_HH

#include "pds/service/Timer.hh"

namespace Pds {

  class VmonTreeMenu;
  class MonTabMenu;

  class VmonMain : public Timer {
  public:
    VmonMain(Task* workTask, 
	     unsigned char platform, 
	     const char* partition,
	     const char* path);
    virtual ~VmonMain();

  private:
    // Main timer stuff
    Task* _task;
    virtual Task*      task()             {return _task;}
    virtual unsigned   duration()   const {return 1000;}
    virtual void       expired();
    virtual unsigned   repetitive() const {return 1;}

  private:
    VmonTreeMenu* _trees;
    MonTabMenu*  _tabs;
  };
};

#endif
