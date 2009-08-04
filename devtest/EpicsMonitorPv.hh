#ifndef EPICS_MONITOR_PV_H
#define EPICS_MONITOR_PV_H

#include <vector>
#include <string>

#include <alarm.h>
#include <epicsTime.h>
#include "cadef.h"
#include "EpicsDbrTools.hh"
#include "EpicsPvData.hh"

namespace Pds
{
    
class EpicsMonitorPv
{
public:
    EpicsMonitorPv() : _bConnected(false), _iPvId(-1), _chidPv(NULL), _lDbfType(-1), _evidCtrl(NULL), _evidTime(NULL),
      _pTimeValue(NULL), _pCtrlValue(NULL), _bTimeValueUpdated(false), _bCtrlValueUpdated(false), 
      _bCtrlValueWritten(false), _lDbrLastUpdateType(-1)
    {}
    
    ~EpicsMonitorPv(); // non-virtual destructor: this class is not for inheritance
    
    /**
     * use compiler-generated copy constructor and copy assignment operator
     */
    
    int init(int iPvId, const std::string& sPvName);
    int release();
    int printPv() const;    
    int writeXtc( char* pcXtcMem, int& iSizeXtc );    
    
    /* Get & Set functions */
    const std::string&  getPvName() const
    {
        return _sPvName;
    }
        
    const int getPvTypeId() const
    {
        return _lDbfType;
    }
    
    bool isConnected() const
    {
        return _bConnected;
    }
    
    /*
       Class usage control:
         Value semantics is only for array initialization use, not really supporting object copies.
    */
    EpicsMonitorPv( const EpicsMonitorPv& epicsMonitorPv1 )
    {
        if ( epicsMonitorPv1.isConnected() )
            throw std::string( "EpicsMonitorPv::EpicsMonitorPv( EpicsMonitorPv& ): Multiple copies of an initialized pv is not allowed\n" );
        new (this) EpicsMonitorPv();
    }
    
    EpicsMonitorPv& operator=(const EpicsMonitorPv& epicsMonitorPv1 )
    {
        if ( isConnected() )
            throw std::string( "EpicsMonitorPv::operator=(): Re-assignment of initialized pvs are not allowed\n" );
        if ( epicsMonitorPv1.isConnected() )
            throw std::string( "EpicsMonitorPv::operator=(): Multiple copies of an initialized pv is not allowed\n" );
        return *this; // do nothing
    }    
    
private:
    static void caConnectionHandler( struct connection_handler_args args );
    static void caSubscriptionHandler(evargs args);
    
    int onCaChannelConnected();
    int onCaChannelDisconnected();
    void onSubscriptionUpdate(const evargs& args);
        
    template <int iDbrType> 
    int printPvByDbrId() const;
        
    template <int iDbrType> 
    int writeXtcByDbrId(char*& pcXtcMem) const;

    template <int iDbrType> 
    int writeXtcCtrlValueByDbrId( char*& pcXtcMem, int& iSizeXtc ) const;

    template <int iDbrType> 
    int writeXtcTimeValueByDbrId( char*& pcXtcMem, int& iSizeXtc ) const;
    
    static const int _iCaChannelPriority = 50; // 0-100    

    bool _bConnected;    
    int _iPvId;
    chid _chidPv;
    std::string _sPvName;
    unsigned long _ulNumElems;
    int _iCaStatus;
    
    long _lDbfType;
    evid _evidCtrl;
    evid _evidTime;
    long _lDbrTimeType;    
    long _lDbrCtrlType;
    void* _pTimeValue;
    void* _pCtrlValue;
    bool _bTimeValueUpdated;
    bool _bCtrlValueUpdated;
    bool _bCtrlValueWritten;
    long _lDbrLastUpdateType;
    
    static const int _iSizeDbrTypes = EpicsDbrTools::iSizeDbrTypes;
    typedef int (EpicsMonitorPv::*TPrintPvFuncPointer)() const;
    static const TPrintPvFuncPointer lfuncPrintPvFunctionTable[_iSizeDbrTypes];
    
    typedef int ( EpicsMonitorPv::* TWriteXtcValueFuncPointer ) ( char*& pcXtcMem, int& iSizeXtc ) const;
    static const TWriteXtcValueFuncPointer lfuncWriteXtcTimeValueFunctionTable[_iSizeDbrTypes];
    static const TWriteXtcValueFuncPointer lfuncWriteXtcCtrlValueFunctionTable[_iSizeDbrTypes];
        
    typedef EpicsDbrTools::Int16 Int16;
    
};

typedef std::vector<EpicsMonitorPv> TEpicsMonitorPvList;
    
template <int iDbrType> 
int EpicsMonitorPv::printPvByDbrId() const
{
    typedef typename EpicsDbrTools::DbrTypeTraits<iDbrType>::TDbrOrg TDbrOrg;
    typedef typename EpicsDbrTools::DbrTypeTraits<iDbrType>::TDbrTime TDbrTime;
    typedef typename EpicsDbrTools::DbrTypeTraits<iDbrType>::TDbrCtrl TDbrCtrl;

    const TDbrTime& pvTimeVal = *(TDbrTime*) _pTimeValue;
    const TDbrCtrl& pvCtrlVal = *(TDbrCtrl*) _pCtrlValue;

    // dbf_text[] contains the name string of each type
    // e.g.     "DBF_STRING", "DBF_SHORT", ...    
    printf( "Type: %s\n", dbf_text[iDbrType+1] + 4 ); 
        
    if ( _ulNumElems > 1 ) printf("Length: %lu\n", _ulNumElems);
        
    char* pValue = NULL;
    unsigned short int usStatus = 0;
    unsigned short int usSeverity = 0;
    if ( _lDbrLastUpdateType == _lDbrTimeType )
    {
        pValue = (char*) &pvTimeVal.value;
        usStatus = pvTimeVal.status;
        usSeverity = pvTimeVal.severity;
    }
    else if ( _lDbrLastUpdateType == _lDbrCtrlType )
    {
        pValue = (char*) &pvCtrlVal.value;
        usStatus = pvCtrlVal.status;
        usSeverity = pvCtrlVal.severity;
    }
    
    printf("Status: %s\n", epicsAlarmConditionStrings[usStatus] );
    printf("Severity: %s\n", epicsAlarmSeverityStrings[usSeverity] );

    if ( _bTimeValueUpdated )
    {
        static const char timeFormatStr[40] = "%04Y-%02m-%02d %02H:%02M:%02S.%03f"; /* Time format string */    
        char sTimeText[40];
        epicsTimeToStrftime(sTimeText, sizeof(sTimeText), timeFormatStr, &pvTimeVal.stamp);
        printf( "TimeStamp: %s\n", sTimeText );
    }
    
    if ( _bCtrlValueUpdated )
        EpicsDbrTools::printCtrlFields(pvCtrlVal);
    
    printf( "Value: " );
    for ( unsigned long iElement = 0; iElement < _ulNumElems; iElement++ )
    {
        EpicsDbrTools::printValue( (TDbrOrg*) pValue);
        pValue += sizeof(TDbrOrg);
        if ( iElement < _ulNumElems-1 ) printf( ", " );
    }
    printf( "\n" );     
        
    return 0;
} // int EpicsMonitorPv::printPvByDbrId() const

template <int iDbrType> 
int EpicsMonitorPv::writeXtcTimeValueByDbrId( char*& pcXtcMem, int& iSizeXtc ) const
{
    new (pcXtcMem) EpicsPvTime<iDbrType>( _iPvId, _ulNumElems, _pTimeValue, &iSizeXtc );    
    
    return 0;
} // int EpicsMonitorPv::writeXtcByDbrId() const

template <int iDbrType>
int EpicsMonitorPv::writeXtcCtrlValueByDbrId( char*& pcXtcMem, int& iSizeXtc ) const
{
    new (pcXtcMem) EpicsPvCtrl<iDbrType>( _iPvId, _ulNumElems, _sPvName.c_str(), _pCtrlValue, &iSizeXtc );
    
    return 0;
} // int EpicsMonitorPv::writeXtcByDbrId() const

} // namespace Pds

#endif
