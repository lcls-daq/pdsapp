#include "XampsConfig.hh"

#include "pdsapp/config/Parameters.hh"
#include "pdsapp/config/ParameterSet.hh"
#include "pdsapp/config/BitCount.hh"
#include "pds/config/XampsConfigType.hh"

#include <new>

using namespace Pds_ConfigDb;

namespace Pds_ConfigDb {

  XampsChannelData::XampsChannelData() {
    for (uint32_t i=0; i<XampsChannel::NumberOfChannelBitFields; i++) {
      _reg[i] = new NumericInt<uint32_t>(
          XampsChannel::name((XampsChannel::ChannelBitFields) i),
          XampsChannel::defaultValue((XampsChannel::ChannelBitFields) i),
          XampsChannel::rangeLow((XampsChannel::ChannelBitFields) i),
          XampsChannel::rangeHigh((XampsChannel::ChannelBitFields) i),
          Decimal
      );
    };
  }

  void XampsChannelData::insert(Pds::LinkedList<Parameter>& pList) {
    for (uint32_t i=0; i<XampsChannel::NumberOfChannelBitFields; i++) {
      pList.insert(_reg[i]);
    }
  }

  int XampsChannelData::pull(void* from) { // pull "from xtc"
    XampsChannel& xampsChannel = *new(from) XampsChannel;
    for (uint32_t i=0; i<XampsChannel::NumberOfChannelBitFields; i++) {
      _reg[i]->value = xampsChannel.get((XampsChannel::ChannelBitFields) i);
    }
    return true;
  }

  int XampsChannelData::push(void* to) {
    XampsChannel& xampsChannel = *new(to) XampsChannel;
    for (uint32_t i=0; i<XampsChannel::NumberOfChannelBitFields; i++) {
      xampsChannel.set((XampsChannel::ChannelBitFields) i, _reg[i]->value);
    }
    return sizeof(XampsChannel);
  }

  XampsASICdata::XampsASICdata()  :
            _count(XampsASIC::NumberOfChannels),
            _channelSet("Channels", _channelArgs, _count) {
    for (uint32_t i=0; i<XampsASIC::NumberOfASIC_Entries; i++){
      _reg[i] = new NumericInt<uint32_t>(
          XampsASIC::name((XampsASIC::ASIC_Entries) i),
          XampsASIC::defaultValue((XampsASIC::ASIC_Entries) i),
          XampsASIC::rangeLow((XampsASIC::ASIC_Entries) i),
          XampsASIC::rangeHigh((XampsASIC::ASIC_Entries) i),
          Decimal
      );
    }
    for (uint32_t i=0; i<XampsASIC::NumberOfChannels; i++) {
      _channel[i] = new XampsChannelData();
      _channel[i]->insert(_channelArgs[i]);
    }
    _channelSet.name("Channel:");
  }

  void XampsASICdata::insert(Pds::LinkedList<Parameter>& pList) {
    for (uint32_t i=0; i<XampsASIC::NumberOfASIC_Entries; i++) {
      pList.insert(_reg[i]);
    }
    pList.insert(&_channelSet);
  }

  int XampsASICdata::pull(void* from) { // pull "from xtc"
    XampsASIC& xampsAsic = *new(from) XampsASIC;
    for (uint32_t i=0; i<XampsASIC::NumberOfASIC_Entries; i++) {
      _reg[i]->value = xampsAsic.get((XampsASIC::ASIC_Entries) i);
    }
    for (uint32_t i=0; i<XampsASIC::NumberOfChannels; i++) {
      _channel[i]->pull(&(xampsAsic.channels()[i]));
    }
    return true;
  }

  int XampsASICdata::push(void* to) {
    XampsASIC& xampsAsic = *new(to) XampsASIC;
    for (uint32_t i=0; i<XampsASIC::NumberOfASIC_Entries; i++) {
      xampsAsic.set((XampsASIC::ASIC_Entries) i, _reg[i]->value);
    }
    for (uint32_t i=0; i<XampsASIC::NumberOfChannels; i++) {
      _channel[i]->push(&(xampsAsic.channels()[i]));
    }
    return sizeof(XampsASIC);
  }

  XampsConfig::XampsConfig() :
                Serializer("Xamps_Config"),
                _count(XampsConfigType::NumberOfASICs),
                _asicSet("ASICs", _asicArgs, _count) {
    for (uint32_t i=0; i<XampsConfigType::NumberOfRegisters; i++) {
      _reg[i] = new NumericInt<uint32_t>(
          XampsConfigType::name((XampsConfigType::Registers) i),
          XampsConfigType::defaultValue((XampsConfigType::Registers) i),
          XampsConfigType::rangeLow((XampsConfigType::Registers) i),
          XampsConfigType::rangeHigh((XampsConfigType::Registers) i),
          Decimal
      );
      pList.insert(_reg[i]);
    }
    pList.insert(&_asicSet);
    for (uint32_t i=0; i<XampsConfigType::NumberOfASICs; i++) {
      _asic[i] = new XampsASICdata();
      _asic[i]->insert(_asicArgs[i]);
    }
    _asicSet.name("ASIC:");
    name("XAMPS Configuration");
  }

  int XampsConfig::readParameters(void* from) { // pull "from xtc"
    XampsConfigType& xampsConf = *new(from) XampsConfigType;
    for (uint32_t i=0; i<XampsConfigType::NumberOfRegisters; i++) {
      _reg[i]->value = xampsConf.get((XampsConfigType::Registers) i);
    }
    for (uint32_t i=0; i<XampsConfigType::NumberOfASICs; i++) {
      _asic[i]->pull(&(xampsConf.ASICs()[i]));
    }
    return sizeof(XampsConfigType);
  }

  int XampsConfig::writeParameters(void* to) {
    XampsConfigType& xampsConf = *new(to) XampsConfigType();
    for (uint32_t i=0; i<XampsConfigType::NumberOfRegisters; i++) {
      xampsConf.set((XampsConfigType::Registers) i, _reg[i]->value);
    }
    for (uint32_t i=0; i<XampsConfigType::NumberOfASICs; i++) {
      _asic[i]->push(&(xampsConf.ASICs()[i]));
    }
    return sizeof(XampsConfigType);
  }

  int XampsConfig::dataSize() const { return sizeof(XampsConfigType); }

};

#include "Parameters.icc"
