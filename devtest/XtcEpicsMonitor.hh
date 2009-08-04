#ifndef XTC_EPICS_MONITOR_H
#define XTC_EPICS_MONITOR_H

#include <string>
#include "pdsdata/xtc/TypeId.hh"
#include "pdsdata/xtc/Level.hh"
#include "pdsdata/xtc/Src.hh"
#include "EpicsMonitorPv.hh"

namespace Pds
{
    
class XtcEpicsMonitor
{
public:
    XtcEpicsMonitor( char* lcFnXtc, int iNumPv, char* lsPvName[] );
    ~XtcEpicsMonitor();
    int runMonitorLoop();
    
    static const int iXtcVersion = 1;    
    static const Src srcLevel;
    static const int iMaxXtcSize = sizeof(EpicsPvCtrl<DBR_DOUBLE>) * 1200; // Space enough for 1000+ PVs of type DBR_DOUBLE
    static const TypeId::Type typeXtc = TypeId::Any;
    
private:        
    std::string _sFnXtc;
    FILE* _fhXtc;
    TEpicsMonitorPvList _lpvPvList;     
    
    static int setupPvList(int iNumPv, char* lsPvName[], TEpicsMonitorPvList& lpvPvList);
    
    // Class usage control: Value semantics is disabled
    XtcEpicsMonitor( const XtcEpicsMonitor& );
    XtcEpicsMonitor& operator=(const XtcEpicsMonitor& );    
};

} // namespace Pds

#endif
