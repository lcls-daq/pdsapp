//
//  Replaces segment level for pyrogue -> AMI
//

#include "pdsapp/python/Epix100aProcessor.hh"
#include "pdsdata/psddl/epix.ddl.h"

typedef Pds::Epix::Config100aV2       EpixConfig;
typedef Pds::Epix::ElementV3          EpixData;

using namespace Pds;

Epix100aProcessor::Epix100aProcessor() :
  _rows(708), _columns(768), _bitdepth(14), _offset(32),
  _srcInfo(0,DetInfo::CxiEndstation,0,DetInfo::Epix100a,0),
  _cfgb(0)
{}
  
Epix100aProcessor::~Epix100aProcessor() { if (_cfgb) delete[] _cfgb; }

Dgram* Epix100aProcessor::configure(Dgram*         dg) {
  insert(dg,TransitionId::Configure);
  Xtc* seg = &dg->xtc;
  Xtc* src = new((char*)seg->next())
    Xtc(TypeId(TypeId::Type(EpixConfig::TypeId),EpixConfig::Version), _srcInfo);
  EpixConfig& cfg = *new((char*)src->next())   
    EpixConfig(-1, // arg__version, 
               0, // arg__usePgpEvr, 
               0, // arg__evrRunCode, 
               0, // arg__evrDaqCode, 
               0, // arg__evrRunTrigDelay, 
               0, // arg__epixRunTrigDelay, 
               0, // arg__dacSetting, 
               0, // arg__asicGR, 
               0, // arg__asicAcq, 
               0, // arg__asicR0, 
               0, // arg__asicPpmat, 
               0, // arg__asicPpbe, 
               0, // arg__asicRoClk, 
               0, // arg__asicGRControl, 
               0, // arg__asicAcqControl, 
               0, // arg__asicR0Control, 
               0, // arg__asicPpmatControl, 
               0, // arg__asicPpbeControl, 
               0, // arg__asicR0ClkControl, 
               0, // arg__prepulseR0En, 
               0, // arg__adcStreamMode, 
               0, // arg__testPatternEnable, 
               0, // arg__SyncMode, 
               0, // arg__R0Mode, 
               0, // arg__acqToAsicR0Delay, 
               0, // arg__asicR0ToAsicAcq, 
               0, // arg__asicAcqWidth, 
               0, // arg__asicAcqLToPPmatL, 
               0, // arg__asicPPmatToReadout, 
               0, // arg__asicRoClkHalfT, 
               0, // arg__adcReadsPerPixel, 
               0, // arg__adcClkHalfT, 
               0, // arg__asicR0Width, 
               0, // arg__adcPipelineDelay, 
               0, // arg__adcPipelineDelay0, 
               0, // arg__adcPipelineDelay1, 
               0, // arg__adcPipelineDelay2, 
               0, // arg__adcPipelineDelay3, 
               0, // arg__SyncWidth, 
               0, // arg__SyncDelay, 
               0, // arg__prepulseR0Width, 
               0, // arg__prepulseR0Delay, 
               0, // arg__digitalCardId0, 
               0, // arg__digitalCardId1, 
               0, // arg__analogCardId0, 
               0, // arg__analogCardId1, 
               0, // arg__carrierId0, 
               0, // arg__carrierId1, 
               2, // arg__numberOfAsicsPerRow, 
               2, // arg__numberOfAsicsPerColumn, 
               352, // arg__numberOfRowsPerAsic, 
               352, // arg__numberOfReadableRowsPerAsic, 
               384, // arg__numberOfPixelsPerAsicRow, 
               2, // arg__calibrationRowCountPerASIC, 
               1, // arg__environmentalRowCountPerASIC, 
               0, // arg__baseClockFrequency, 
               0xf, // arg__asicMask, 
               0, // arg__enableAutomaticRunTrigger, 
               0, // arg__numberOf125MhzTicksPerRunTrigger, 
               0, // arg__scopeEnable, 
               0, // arg__scopeTrigEdge, 
               0, // arg__scopeTrigChan, 
               0, // arg__scopeArmMode, 
               0, // arg__scopeADCThreshold, 
               0, // arg__scopeTrigHoldoff, 
               15, // arg__scopeTrigOffset, 
               4096, // arg__scopeTraceLength, 
               0, // arg__scopeADCsameplesToSkip, 
               0, // arg__scopeChanAwaveformSelect, 
               4, // arg__scopeChanBwaveformSelect, 
               0, // const Epix::Asic100aConfigV1* arg__asics, 
               0, // const uint16_t* arg__asicPixelConfigArray, 
               0  // const uint8_t * arg__calibPixelConfigArray
               );
  if (_cfgb) delete _cfgb;
  _cfgb = new char[cfg._sizeof()];
  memcpy(_cfgb, (char*)&cfg, cfg._sizeof());

  src->alloc(cfg._sizeof());
  seg->alloc(cfg._sizeof()+sizeof(Xtc));
  return dg;
}

Dgram* Epix100aProcessor::event(Dgram*      dg,
                                const char* source) {
  insert(dg,TransitionId::L1Accept);
  Xtc* seg = &dg->xtc;
  const EpixConfig& cfg = *reinterpret_cast<const EpixConfig*>(_cfgb);
  Xtc* src = new((char*)seg->alloc(EpixData::_sizeof(cfg)+sizeof(Xtc)))
    Xtc(TypeId(TypeId::Type(EpixData::TypeId),EpixData::Version), _srcInfo);
  char* data = (char*)src->alloc(EpixData::_sizeof(cfg));
  memcpy(data, source, _offset); // PGP header
  data += _offset; source += _offset;
  { 
    for(unsigned i=0; i<_rows/2; i++) {
      const uint16_t* ru = reinterpret_cast<const uint16_t*>(source)+_columns*(i*2+1);
      const uint16_t* rd = reinterpret_cast<const uint16_t*>(source)+_columns*(i*2+0);
      uint16_t* qu = reinterpret_cast<uint16_t*>(data)+_columns*(_rows/2-i-1);
      uint16_t* qd = reinterpret_cast<uint16_t*>(data)+_columns*(_rows/2+i);
      for(unsigned j=0; j<_columns; j++) {
        qu[j] = ru[j];
        qd[j] = rd[j];
      }
    }
  }
  data   += _rows*_columns*2;
  source += _rows*_columns*2;
  memcpy(data, source, EpixData::_sizeof(cfg)-_rows*_columns*2-_offset);
  return dg;
}
