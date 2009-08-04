#ifndef XTC_EPICS_ITERATOR_H
#define XTC_EPICS_ITERATOR_H

#include "pdsdata/xtc/XtcIterator.hh"

namespace Pds
{
    
class XtcEpicsIterator : 
  public XtcIterator 
{
public:
    XtcEpicsIterator(Xtc* xtc, unsigned int iDepth) : XtcIterator(xtc), _iDepth(iDepth) {}
    virtual int process(Xtc* xtc);
private:
    unsigned int _iDepth;
};    
    
} // namespace Pds 

#endif
