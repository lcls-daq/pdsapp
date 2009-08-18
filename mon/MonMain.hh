#ifndef Pds_MonMAIN_HH
#define Pds_MonMAIN_HH

#include "pds/service/Timer.hh"

namespace Pds {

  class MonTreeMenu;
  class MonTabMenu;

  class MonMain : public Timer {
  public:
    MonMain(Task* workTask, const char** hosts, const char* config);
    virtual ~MonMain();

  private:
    // Main timer stuff
    Task* _task;
    virtual Task*      task()             {return _task;}
    virtual unsigned   duration()   const {return 220;}
    virtual void       expired();
    virtual unsigned   repetitive() const {return 1;}

  private:
    MonTreeMenu* _trees;
    MonTabMenu*  _tabs;
  };
};

#endif
