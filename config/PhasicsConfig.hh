#ifndef Pds_PhasicsConfig_hh
#define Pds_PhasicsConfig_hh

#include "pdsapp/config/Serializer.hh"
#include "pds/config/PhasicsConfigType.hh"
#include "pdsapp/config/Parameters.hh"
#include "pdsapp/config/ParameterSet.hh"
#include "pdsapp/config/BitCount.hh"

#include <QtGui/QPushButton>
#include <QtCore/QObject>

namespace Pds_ConfigDb {

//  class PhasicsSimpleCount : public ParameterCount {
//    public:
//    PhasicsSimpleCount(unsigned c) : mycount(c) {};
//    ~PhasicsSimpleCount() {};
//    bool connect(ParameterSet&) { return false; }
//    unsigned count() { return mycount; }
//    unsigned mycount;
//  };


  class PhasicsConfig : public Serializer {
  public:
    PhasicsConfig();
    ~PhasicsConfig() {};
  public:
    int  readParameters (void* from);
    int  writeParameters(void* to);
    int  dataSize       () const;
  public:
    NumericInt<uint32_t>*       _reg[PhasicsConfigType::NumberOfRegisters];
//    PhasicsSimpleCount           _count;
  };

};

#endif
