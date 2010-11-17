#include "pdsapp/monobs/CspadMon.hh"
#include "pdsapp/monobs/ShmClient.hh"
#include "pdsapp/monobs/Handler.hh"
#include "pds/config/CsPadConfigType.hh"
#include "pds/config/EvrConfigType.hh"
#include "pdsdata/xtc/DetInfo.hh"
#include "pdsdata/xtc/ClockTime.hh"
#include "pdsdata/xtc/Xtc.hh"
#include "pdsdata/cspad/ElementIterator.hh"

#include <math.h>
#include <string>

static const unsigned MaxSections=32;
static const unsigned BYKIK=162;
static const Pds::DetInfo evrInfo(0,Pds::DetInfo::NoDetector, 0, Pds::DetInfo::Evr, 0);

typedef float    CspadSection  [Pds::CsPad::ColumnsPerASIC][2*Pds::CsPad::MaxRowsPerASIC];
typedef unsigned CspadStatusMap[Pds::CsPad::ColumnsPerASIC][2*Pds::CsPad::MaxRowsPerASIC];

namespace PdsCas {
  class CspadPixelStatus {
  public:
    enum {VeryHot=1,
          Hot=2,
          Cold=4};
    CspadPixelStatus() : _badFlags(1) {}
    ~CspadPixelStatus() {}
  public:
    bool ok(unsigned col, unsigned row) const 
    { return ((_section[col][row] & _badFlags) == 0); }
    void setBadFlags(unsigned flag) { _badFlags=flag; }
  public:
    unsigned _badFlags;
    CspadStatusMap _section;
  };

  class EvrHandler : public Handler {
  public:
    EvrHandler() :
      Handler(evrInfo, Pds::TypeId::Id_EvrData, Pds::TypeId::Id_EvrConfig) {}
    ~EvrHandler() {}
  public:
    void   _configure(const void* payload, const Pds::ClockTime& t) {}
    void   _event    (const void* payload, const Pds::ClockTime& t)
    {
      const EvrDataType& d = *reinterpret_cast<const EvrDataType*>(payload);
      for(unsigned i=0; i<d.numFifoEvents(); i++)
        if (d.fifoEvent(i).EventCode==BYKIK) {
          _bykik = true;
          return;
        }
      _bykik = false;
    }
    void   _damaged  () {}
  public:
    void   initialize() {}
    void   update_pv () {}
  public:
    bool _bykik;
  };

  class CspadHandler : public Handler {
  public:
    CspadHandler(const Pds::Src&     info) :
      Handler(info, Pds::TypeId::Id_CspadElement, Pds::TypeId::Id_CspadConfig)
    {
      _offsets = new CspadSection    [MaxSections];
      _result  = new CspadSection    [MaxSections];
      _status  = new CspadPixelStatus[MaxSections];
    
      sprintf(_statusMap,"/tmp/cspad.%08x.sta",info.phy());
      sprintf(_pedFile  ,"/tmp/cspad.%08x.dat",info.phy());
    }
    ~CspadHandler() {}
  public:
    void   _configure(const void* payload, const Pds::ClockTime& t) 
    {
      _config = *reinterpret_cast<const Pds::CsPad::ConfigV2*>(payload);
      _loadConstants(); 
    }
    void   _event    (const void* payload, const Pds::ClockTime& t) 
    { 
      _payload=payload;
    }
  public:
    void  process()
    {
      if (_validConstants) {
        const Xtc& xtc = *reinterpret_cast<const Xtc*>((const char*)_payload-sizeof(Xtc));
        Pds::CsPad::ElementIterator iter(_config,xtc);
        while( iter.next() ) {
          unsigned section_id;
          const Pds::CsPad::Section* s;
          while((s=iter.next(section_id))) {
            try {
              float mean_rms = _getPedestalConsistency(*s, _offsets[section_id], _status[section_id]);
              //  Fill a PV array with these values?
              printf("section %d : pedrms %g\n",section_id,mean_rms);
            }
            catch (std::string errMsg) {
            }
          }
        }
      }
    }
    void   _damaged  () {}
  public:
    void   initialize() {}
    void   update_pv () {}
  private:
    void   _loadConstants() {
      _validConstants=false;

      FILE* status = fopen(_statusMap, "r");
      if (status) {
        printf("Loading status map from %s\n", _statusMap);
        char* linep = NULL;
        size_t sz = 0;
        char* sEnd;
        for (unsigned section=0; section < MaxSections; section++) {
          CspadStatusMap& s = _status[section]._section;
          for (unsigned col=0; col < Pds::CsPad::ColumnsPerASIC; col++) {
            getline(&linep, &sz, status);
            s[col][0] = strtoul(linep, &sEnd, 0);
            for (unsigned row=1; row < 2*Pds::CsPad::MaxRowsPerASIC; row++)
              s[col][row] = strtoul(sEnd, &sEnd, 0);
          }
        }
      }
      else {
        printf("Failed to open %s, no valid status map\n", _statusMap);
        return;
      }

      FILE* peds = fopen(_pedFile, "r");
      if (peds) {
        printf("Loading pedestal map from %s\n", _pedFile);
        char* linep = NULL;
        size_t sz = 0;
        char* pEnd;
        for (unsigned section=0; section < MaxSections; section++) {
          CspadSection& p = _offsets[section];
          for (unsigned col=0; col < Pds::CsPad::ColumnsPerASIC; col++) {
            getline(&linep, &sz, peds);
            p[col][0] = strtod(linep, &pEnd);
            for (unsigned row=1; row < 2*Pds::CsPad::MaxRowsPerASIC; row++)
              p[col][row] = strtod(pEnd, &pEnd);
          }
        }
      }
      else {
        printf("Failed to open %s, no valid pedestal map\n", _pedFile);
        return;
      }
      _validConstants = true;
    }

    float  _getPedestalConsistency(const Pds::CsPad::Section& s,
                                   const CspadSection&        o,
                                   const CspadPixelStatus&    m)
    {
      double s0 = 0.;
      double s1 = 0.;
      double s2 = 0.;
      for(unsigned col=0; col<Pds::CsPad::ColumnsPerASIC; col++) {
        for(unsigned row=0; row<2*Pds::CsPad::MaxRowsPerASIC; row++) {
          if (not m.ok(col, row)) continue; // simple if slow
          float val = float(s.pixel[col][row]) - o[col][row];
          s0++;
          s1 += val;
          s2 += val*val;
        }
      }
      return sqrt(s0*s2 - s1*s1)/s0;
    }
  private:
    CspadSection*     _offsets;  // current valid dark frame offsets
    CspadSection*     _result ;  // we keep ownership of result array
    CspadPixelStatus* _status ;
    char              _statusMap[64];
    char              _pedFile  [64];
    bool              _validConstants;
    const void*       _payload;
    Pds::CsPad::ConfigV2 _config;
  };

  class CspadEHandler : public EvtHandler {
  public:
    CspadEHandler(const EvrHandler& evr,
                  CspadHandler& cspad) :
      _evr  (evr),
      _cspad(cspad) {}
  public:
    void   _configure(const Pds::ClockTime& t) {}
    void   _event    (const Pds::ClockTime& t) {
      if (_evr._bykik)
        _cspad.process();
    }
    void   _damaged  () {}
  public:
    void         initialize() {}
    void         update_pv () {}
  private:
    const EvrHandler& _evr;
    CspadHandler& _cspad;
  };

};

void PdsCas::CspadMon::monitor(ShmClient& client,
                               const DetInfo& det)
{
  EvrHandler* evr = new EvrHandler;
  CspadHandler* cspad = new CspadHandler(det);
  client.insert(evr);
  client.insert(cspad);
  client.insert(new CspadEHandler(*evr,*cspad));
}
