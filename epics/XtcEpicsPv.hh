#ifndef XTC_EPICS_PV_H
#define XTC_EPICS_PV_H
#include "cadef.h"
#include "pds/xtc/Datagram.hh"
#include "pds/service/GenericPool.hh"
#include "pdsdata/xtc/TypeId.hh"
#include "EpicsMonitorPv.hh"

namespace Pds
{
    
class XtcEpicsPv : public Xtc
{
public:
    XtcEpicsPv(TypeId typeId, Src src) : Xtc(typeId,src)
    {
        // Standard adjustment: Update the Xtc size field
        alloc( sizeof(*this) - sizeof(Xtc) );
    }  

    // only support new operator with Xtc pointer, to be used for Xtc writing
    static void* operator new(size_t size, Xtc* pXtc)
    { 
        _pGlobalXtc = pXtc;
        return Xtc::operator new(size, pXtc);
    }
    
    int setValue(EpicsMonitorPv& epicsPv );
    
private:
    static Xtc* _pGlobalXtc; // global Xtc that contains ALL epics pv xtcs
    
    // Class usage control: Value semantics is disabled
    XtcEpicsPv( const XtcEpicsPv& );
    XtcEpicsPv& operator=(const XtcEpicsPv& );    
};



} // namespace Pds


#endif

