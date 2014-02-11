#include <stdio.h>
#include <string>
#include "pds/xtc/Datagram.hh"
#include "pds/service/GenericPool.hh"
#include "pdsdata/xtc/TypeId.hh"
#include "XtcEpicsPv.hh"
#include "console_io.hh"
#include "XtcEpicsMonitor.hh"

namespace Pds
{
    
using std::string;

const Src XtcEpicsMonitor::srcLevel(Level::Recorder); // For direct file recording
    
XtcEpicsMonitor::XtcEpicsMonitor( char* lcFnXtc, int iNumPv, char* lsPvName[] ) :
    _sFnXtc(lcFnXtc), _fhXtc(NULL)
{
    // initialize Channel Access        
    int iFail = ca_task_initialize();    
    if (ECA_NORMAL != iFail ) 
    {
        SEVCHK( iFail, "XtcEpicsMonitor::XtcEpicsMonitor(): ca_task_initialize() failed" );    
        throw "XtcEpicsMonitor::XtcEpicsMonitor(): ca_task_initialize() failed";
    }
  
    if ( lcFnXtc == NULL || iNumPv <= 0 || lsPvName == NULL )
        throw string("XtcEpicsMonitor::XtcEpicsMonitor(): Invalid parameters");
        
    _fhXtc = fopen(_sFnXtc.c_str(),"wb");
    if ( _fhXtc == NULL )
        throw string("XtcEpicsMonitor::XtcEpicsMonitor()::fopen(%s) failed\n", _sFnXtc.c_str());     
                
    iFail = setupPvList( iNumPv, lsPvName, _lpvPvList );
    if (iFail != 0)
        throw string("XtcEpicsMonitor::XtcEpicsMonitor()::setupPvList() Failed");        
}

XtcEpicsMonitor::~XtcEpicsMonitor()
{
    for ( int iPvName = 0; iPvName < (int) _lpvPvList.size(); iPvName++ )
    {           
        EpicsMonitorPv& epicsPvCur = _lpvPvList[iPvName];
        int iFail = epicsPvCur.release();
        
        if (iFail != 0)
          printf( "xtcEpicsTest()::EpicsMonitorPv::release(%s (%s)) failed\n", 
            epicsPvCur.getPvDescription().c_str(), epicsPvCur.getPvName().c_str());
    }
    
    if (_fhXtc != NULL)
    {
        fclose(_fhXtc);
        _fhXtc = NULL;
    }
    
    int iFail = ca_task_exit();
    if (ECA_NORMAL != iFail ) 
      SEVCHK( iFail, "XtcEpicsMonitor::~XtcEpicsMonitor(): ca_task_exit() failed" );    
}

int XtcEpicsMonitor::runMonitorLoop()
{
    const int iNumPv = _lpvPvList.size();
    
    GenericPool* pPool = new GenericPool(XtcEpicsMonitor::iMaxXtcSize, 1);
    TypeId typeIdXtc(XtcEpicsMonitor::typeXtc, XtcEpicsMonitor::iXtcVersion);        
    
    bool bFirstReport = true;
    /* Keep waiting for CA events */
    while (1)
    {
        ca_poll();
        
        Datagram* pDatagram = new(pPool) Datagram(typeIdXtc, XtcEpicsMonitor::srcLevel);
        char* pPoolBufferOverflowWarning = (char*) pDatagram + (int)(0.9*XtcEpicsMonitor::iMaxXtcSize);
        
        bool bPvUpdated = false;
        printf("\n");
        for ( int iPvName = 0; iPvName < iNumPv; iPvName++ )
        {
            EpicsMonitorPv& epicsPvCur = _lpvPvList[iPvName];
            if ( !epicsPvCur.isConnected() )
            {
                printf( "XtcEpicsMonitor::runMonitorLoop(): PV %s (%s) has not been connected to CA library\n",
                  epicsPvCur.getPvDescription().c_str(), epicsPvCur.getPvName().c_str() );
                continue;
            }
            
            epicsPvCur.printPv();
            
            XtcEpicsPv* pXtcEpicsPvCur = new(&pDatagram->xtc) XtcEpicsPv(typeIdXtc, srcLevel);

            int iFail = pXtcEpicsPvCur->setValue( epicsPvCur, bFirstReport );
            if ( iFail == 0 )
                bPvUpdated = true;
                
            char* pCurBufferPointer = (char*) pDatagram->xtc.next();                
            if ( pCurBufferPointer > pPoolBufferOverflowWarning  )
            {
                printf( "XtcEpicsMonitor::runMonitorLoop(): Pool buffer size is too small.\n" );
                printf( "XtcEpicsMonitor::runMonitorLoop():   %d Pvs are stored in 90%% of the pool buffer (size = %d bytes)\n", 
                  iPvName+1, XtcEpicsMonitor::iMaxXtcSize );
                break;
            }
        }        
                
        if ( bPvUpdated )
        {
            /// !! For debug print
            printf("Size of Data: Xtc %zu Datagram %zu Payload %d Total %zu\n", 
                sizeof(Xtc), sizeof(Datagram), pDatagram->xtc.sizeofPayload(), 
                sizeof(Datagram) + pDatagram->xtc.sizeofPayload() );
            fwrite(pDatagram, sizeof(Datagram) + pDatagram->xtc.sizeofPayload(), 1, _fhXtc);
        }            
        delete pDatagram;
        
        if ( bFirstReport ) bFirstReport = false;
        
        printf( "\nContiue monitoring %d PVs? [Y/[nq]]", iNumPv );
        
        char cIntput = 'Y';
        ConsoleIO::kbhit(&cIntput);
        fflush(NULL);
        
        if ( cIntput == 'n' || cIntput == 'N' || cIntput == 'q' || cIntput == 'Q')
            break;    
            
        ca_pend_event(1); 
    }
    
    printf("\n");
    fflush(NULL);  
    
    delete pPool;
                
    return 0;
}

int XtcEpicsMonitor::setupPvList(int iNumPv, char* lsPvName[], TEpicsMonitorPvList& lpvPvList)
{
    const int iNumRepeat = 1; // For testing maximal number of PVs that can be recorded and read correctly
    
    int iOrgNumPv = iNumPv;
    iNumPv *= iNumRepeat;    
    
    lpvPvList.resize(iNumPv);
    for ( int iPvName = 0; iPvName < iNumPv; iPvName++ )
    {                
        EpicsMonitorPv& epicsPvCur = lpvPvList[iPvName];
        //int iFail = epicsPvCur.init( lsPvName[iPvName] );
        
        string sPvName( lsPvName[iPvName % iOrgNumPv] );
        //if ( iPvName != 0 )
        //    sPvName += ( itoa( iPvName) );
        int iFail = epicsPvCur.init( iPvName, sPvName.c_str(), sPvName.c_str(), 1, 1 );
        
        if (iFail != 0)
        {
            printf( "setupPvList()::EpicsMonitorPv::init(%s (%s)) failed\n", 
              epicsPvCur.getPvDescription().c_str(), epicsPvCur.getPvName().c_str());
            return 1;   
        }       
    }    
    
    /* flush all CA monitor requests*/
    ca_flush_io();        
    ca_pend_event(0.2); 
    
    return 0;
}

} // namespace Pds
