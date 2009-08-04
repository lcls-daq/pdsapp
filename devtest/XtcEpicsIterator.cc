#include <stdio.h>
#include <string>
#include "pdsdata/xtc/Xtc.hh"
#include "pdsdata/xtc/Level.hh"
#include "pdsdata/xtc/DetInfo.hh"
#include "pdsdata/xtc/ProcInfo.hh"
#include "XtcEpicsIterator.hh"
#include "XtcEpicsMonitor.hh"
#include "EpicsPvData.hh"

namespace Pds
{

using std::string;

int XtcEpicsIterator::process(Xtc* xtc) 
{    
    printf( string( _iDepth*2, ' ' ).c_str() );
    
    
    Level::Type level = xtc->src.level();    
    printf("%s level: ",Level::name(level));
    
    
    if (level != XtcEpicsMonitor::srcLevel.level()) 
    {
        printf("XtcEpicsIterator::level is not correct (should be %s)\n", Level::name( XtcEpicsMonitor::srcLevel.level() ) );
        return 0; // return Zero to stop recursive processing
    }
    
    TypeId::Type xtcTypeId = xtc->contains.id();
    switch ( xtcTypeId ) 
    {
    case ( TypeId::Id_Xtc ) : 
      {
        XtcEpicsIterator iter( xtc, _iDepth+1 );
        iter.iterate();
        break;
      }
    case ( XtcEpicsMonitor::typeXtc ) :
      {
        EpicsPvHeader* pEpicsPvData = (EpicsPvHeader*) ( xtc->payload() );
        pEpicsPvData->printPv();
        printf( "\n" );
      }
      break;
    default :
        printf("XtcEpicsIterator::type id %s is not correct\n", TypeId::name( xtcTypeId ) );
        return 0; // return Zero to stop recursive processing
    }

    return 1; // return NonZero value to let upper level Xtc continue the processing
}
  
} // namespace Pds
