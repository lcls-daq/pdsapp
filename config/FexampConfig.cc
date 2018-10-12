#include "FexampConfig.hh"

#include "pdsapp/config/Parameters.hh"
#include "pdsapp/config/ParameterSet.hh"
#include "pdsapp/config/BitCount.hh"
#include "pds/config/FexampConfigType.hh"

#include <new>

namespace Pds_ConfigDb {

  void FexampChannelSet::copyClicked() {
    dialog->exec();
  }

  QWidget* FexampChannelSet::insertWidgetAtLaunch(int ind) {
    dialog = new FexampCopyChannelDialog(ind);
    dialog->context(myAsic, config);
    copyButton = new QPushButton(tr("&Copy this Channel"));
    QObject::connect(copyButton, SIGNAL(clicked()), this, SLOT(copyClicked()));
    return copyButton;
  }

  void FexampAsicSet::copyClicked() {
    dialog->exec();
  }

  QWidget* FexampAsicSet::insertWidgetAtLaunch(int ind) {
    dialog = new FexampCopyAsicDialog(ind);
    dialog->context(config);
    copyButton = new QPushButton(tr("&Copy this ASIC"));
    QObject::connect(copyButton, SIGNAL(clicked()), this, SLOT(copyClicked()));
    return copyButton;
  }

  FexampChannelData::FexampChannelData() {
    for (uint32_t i=0; i<FexampChannel::NumberOfChannelBitFields; i++) {
      _reg[i] = new NumericInt<uint32_t>(
          FexampChannel::name((FexampChannel::ChannelBitFields) i),
          FexampChannel::defaultValue((FexampChannel::ChannelBitFields) i),
          FexampChannel::rangeLow((FexampChannel::ChannelBitFields) i),
          FexampChannel::rangeHigh((FexampChannel::ChannelBitFields) i),
          Decimal
      );
    };
  }

  void FexampChannelData::insert(Pds::LinkedList<Parameter>& pList) {
    for (uint32_t i=0; i<FexampChannel::NumberOfChannelBitFields; i++) {
      pList.insert(_reg[i]);
    }
  }

  int FexampChannelData::pull(void* from) { // pull "from xtc"
    FexampChannel& fexampChannel = *new(from) FexampChannel;
    for (uint32_t i=0; i<FexampChannel::NumberOfChannelBitFields; i++) {
      _reg[i]->value = fexampChannel.get((FexampChannel::ChannelBitFields) i);
    }
    return true;
  }

  int FexampChannelData::push(void* to) {
    FexampChannel& fexampChannel = *new(to) FexampChannel;
    for (uint32_t i=0; i<FexampChannel::NumberOfChannelBitFields; i++) {
      fexampChannel.set((FexampChannel::ChannelBitFields) i, _reg[i]->value);
    }
    return sizeof(FexampChannel);
  }

  FexampASICdata::FexampASICdata(FexampConfig* c, int index)  :
            _count(FexampASIC::NumberOfChannels),
            _channelSet("Channels", _channelArgs, _count),
            _config(c) {
    for (uint32_t i=0; i<FexampASIC::NumberOfASIC_Entries; i++){
      _reg[i] = new NumericInt<uint32_t>(
          FexampASIC::name((FexampASIC::ASIC_Entries) i),
          FexampASIC::defaultValue((FexampASIC::ASIC_Entries) i),
          FexampASIC::rangeLow((FexampASIC::ASIC_Entries) i),
          FexampASIC::rangeHigh((FexampASIC::ASIC_Entries) i),
          Decimal
      );
    }
    for (uint32_t i=0; i<FexampASIC::NumberOfChannels; i++) {
      _channel[i] = new FexampChannelData();
      _channel[i]->insert(_channelArgs[i]);
    }
    _channelSet.name("Channel:");
    _channelSet.config = _config;
    _channelSet.myAsic = index;
  }

  void FexampASICdata::insert(Pds::LinkedList<Parameter>& pList) {
    for (uint32_t i=0; i<FexampASIC::NumberOfASIC_Entries; i++) {
      pList.insert(_reg[i]);
    }
    pList.insert(&_channelSet);
  }

  int FexampASICdata::pull(void* from) { // pull "from xtc"
    FexampASIC& fexampAsic = *new(from) FexampASIC;
    for (uint32_t i=0; i<FexampASIC::NumberOfASIC_Entries; i++) {
      _reg[i]->value = fexampAsic.get((FexampASIC::ASIC_Entries) i);
    }
    for (uint32_t i=0; i<FexampASIC::NumberOfChannels; i++) {
      _channel[i]->pull(&(fexampAsic.channels()[i]));
    }
    return true;
  }

  int FexampASICdata::push(void* to) {
    FexampASIC& fexampAsic = *new(to) FexampASIC;
    for (uint32_t i=0; i<FexampASIC::NumberOfASIC_Entries; i++) {
      fexampAsic.set((FexampASIC::ASIC_Entries) i, _reg[i]->value);
    }
    for (uint32_t i=0; i<FexampASIC::NumberOfChannels; i++) {
      _channel[i]->push(&(fexampAsic.channels()[i]));
    }
    return sizeof(FexampASIC);
  }

  FexampConfig::FexampConfig() :
                Serializer("Fexamp_Config"),
                _count(FexampConfigType::NumberOfASICs),
                _asicSet("ASICs", _asicArgs, _count) {
    for (uint32_t i=0; i<FexampConfigType::NumberOfRegisters; i++) {
      _reg[i] = new NumericInt<uint32_t>(
          FexampConfigType::name((FexampConfigType::Registers) i),
          FexampConfigType::defaultValue((FexampConfigType::Registers) i),
          FexampConfigType::rangeLow((FexampConfigType::Registers) i),
          FexampConfigType::rangeHigh((FexampConfigType::Registers) i),
          Decimal
      );
      pList.insert(_reg[i]);
    }
    pList.insert(&_asicSet);
    for (uint32_t i=0; i<FexampConfigType::NumberOfASICs; i++) {
      _asic[i] = new FexampASICdata(this, i);
      _asic[i]->insert(_asicArgs[i]);
    }
    _asicSet.name("ASIC:");
    _asicSet.config = this;
    name("FEXAMP Configuration");
  }

  int FexampConfig::readParameters(void* from) { // pull "from xtc"
    FexampConfigType& fexampConf = *new(from) FexampConfigType;
    for (uint32_t i=0; i<FexampConfigType::NumberOfRegisters; i++) {
      _reg[i]->value = fexampConf.get((FexampConfigType::Registers) i);
    }
    for (uint32_t i=0; i<FexampConfigType::NumberOfASICs; i++) {
      _asic[i]->pull(&(fexampConf.ASICs()[i]));
    }
    return sizeof(FexampConfigType);
  }

  int FexampConfig::writeParameters(void* to) {
    FexampConfigType& fexampConf = *new(to) FexampConfigType();
    for (uint32_t i=0; i<FexampConfigType::NumberOfRegisters; i++) {
      fexampConf.set((FexampConfigType::Registers) i, _reg[i]->value);
    }
    for (uint32_t i=0; i<FexampConfigType::NumberOfASICs; i++) {
      _asic[i]->push(&(fexampConf.ASICs()[i]));
    }
    return sizeof(FexampConfigType);
  }

  int FexampConfig::dataSize() const { return sizeof(FexampConfigType); }

};


