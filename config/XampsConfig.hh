#ifndef Pds_XampsConfig_hh
#define Pds_XampsConfig_hh

#include "pdsapp/config/Serializer.hh"
#include "pds/config/XampsConfigType.hh"
#include "pdsapp/config/Parameters.hh"
#include "pdsapp/config/ParameterSet.hh"
#include "pdsapp/config/BitCount.hh"

namespace Pds_ConfigDb {

  class SimpleCount : public ParameterCount {
    public:
    SimpleCount(unsigned c) : mycount(c) {};
    ~SimpleCount() {};
    bool connect(ParameterSet&) { return false; }
    unsigned count() { return mycount; }
    unsigned mycount;
  };

  class XampsChannelData {
    public:
      XampsChannelData();
      ~XampsChannelData() {};
      void insert(Pds::LinkedList<Parameter>&);
      int pull(void*);
      int push(void*);
    public:
      NumericInt<uint32_t>* _reg[XampsChannel::NumberOfChannelBitFields];
  };

  class XampsASICdata {
    public:
      XampsASICdata();
      ~XampsASICdata() {};
      void insert(Pds::LinkedList<Parameter>&);
      int pull(void*);
      int push(void*);
    public:
      NumericInt<uint32_t>*      _reg[XampsASIC::NumberOfASIC_Entries];
      SimpleCount                _count;
      XampsChannelData*          _channel[XampsASIC::NumberOfChannels];
      Pds::LinkedList<Parameter> _channelArgs[XampsASIC::NumberOfChannels];
      ParameterSet               _channelSet;
  };

  class XampsConfig : public Serializer {
  public:
    XampsConfig();
    ~XampsConfig() {};
  public:
    int  readParameters (void* from);
    int  writeParameters(void* to);
    int  dataSize       () const;
  public:
    NumericInt<uint32_t>*      _reg[XampsConfigType::NumberOfRegisters];
    SimpleCount                _count;
    XampsASICdata*             _asic[XampsConfigType::NumberOfASICs];
    Pds::LinkedList<Parameter> _asicArgs[XampsConfigType::NumberOfASICs];
    ParameterSet               _asicSet;
  };

};

#endif
