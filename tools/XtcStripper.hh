#ifndef Pds_XtcStripper_hh
#define Pds_XtcStripper_hh

//
//  Need an iterator that is safe for backwards copy
//

#include <stdint.h>
#include <stdlib.h>

namespace Pds {
  class Xtc;

  class XtcStripper {
  public:
    enum Status {Stop, Continue};
    XtcStripper(Xtc*, uint32_t*&);
    virtual ~XtcStripper();
  public:
    void iterate();
  protected:
    void _write(const void*, ssize_t);
    void iterate(Xtc* root);
    virtual void process(Xtc* xtc);
  private:
    Xtc*       _root;
    uint32_t*& _pwrite;
  };
};

#endif
