#ifndef Pds_ConfigDb_PdsDefs_hh
#define Pds_ConfigDb_PdsDefs_hh

#include "pdsdata/xtc/DetInfo.hh"
#include "pdsdata/xtc/TypeId.hh"

#include <string>
using std::string;

namespace Pds_ConfigDb {

  //  Unqualified type name; e.g. "Opal1kConfig"
  class UTypeName : public string {  
  public:
    UTypeName() {}
    explicit UTypeName(const string& s) : string(s) {}
  };

  //  Qualified type name; e.g. "Opal1kConfig_v5"
  class QTypeName : public string {
  public:
    QTypeName() {}
    explicit QTypeName(const string& s) : string(s) {}
  };

  class PdsDefs {
  public:
    enum ConfigType { FrameFex, Opal1k, Quartz, TM6740, Evr, EvrIO, 
                      //          Sequencer,
                      AcqADC, AcqTDC,
                      Ipimb, IpmDiode, PimDiode, PimImage, 
                      Encoder, pnCCD, RunControl, Princeton, Fccd, Cspad, Gsc16ai, 
                      Timepix, Cspad2x2, OceanOptics, Fli, Andor, UsdUsb, Orca, Imp,
#ifdef BUILD_EXTRA
                      Phasics, Xamps, Fexamp,
#endif
                      NumberOf };

    static const Pds::TypeId* typeId   (ConfigType);         
    static const Pds::TypeId* typeId   (const UTypeName&);
    static const Pds::TypeId* typeId   (const QTypeName&);
    static UTypeName          utypeName(ConfigType);
    static UTypeName          utypeName(const Pds::TypeId&); 
    static QTypeName          qtypeName(const Pds::TypeId&); 
    static QTypeName          qtypeName(const UTypeName&);
    static ConfigType         configType(const Pds::TypeId&);
  };
};

#endif
