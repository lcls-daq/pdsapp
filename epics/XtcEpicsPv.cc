#include <memory.h>
#include "XtcEpicsPv.hh"

namespace Pds
{
Xtc* XtcEpicsPv::_pGlobalXtc = NULL;
    
int XtcEpicsPv::setValue(EpicsMonitorPv& epicsPv )
{      
    char* pXtcMem = (char*)(this+1);
    int iSizeXtcEpics = 0;
    
    int iFail = epicsPv.writeXtc( pXtcMem, iSizeXtcEpics );
    if ( iFail != 0 )
        return 1;
            
    // Adjust self size
    alloc( iSizeXtcEpics );
    
    // Adjust the size of global Xtc, which contains ALL epics pv xtcs
    _pGlobalXtc->alloc( iSizeXtcEpics );
                
    return 0;
}

} // namespace Pds
