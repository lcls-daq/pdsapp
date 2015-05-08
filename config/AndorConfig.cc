#include "pdsapp/config/AndorConfig.hh"

#include "pdsapp/config/Parameters.hh"
#include "pdsapp/config/QtConcealer.hh"
#include "pds/config/AndorConfigType.hh"
#include <QtGui/QCheckBox>
#include <QtGui/QVBoxLayout>

#include <new>

namespace Pds_ConfigDb {
  class AndorConfig::Private_Data : public Parameter {
  static const char*  lsEnumFanMode[];
  public:
    Private_Data() :
      _uWidth               ("Width",               16, 1,      4152),
      _uHeight              ("Height",              16, 1,      4128),
      _uOrgX                ("Orgin X",             0,    0,    4151),
      _uOrgY                ("Orgin WHY",           0,    0,    4127),
      _uBinX                ("Binning X",           1,    1,    2048),
      _boxBinning           ("Full Binning Mode",   false),
      _uBinY                ("Binning Y",           1,    1,    2048),
      // Note: Here the min exposure time need to set 9.99e-7 to allow user to input 1e-6, due to floating points imprecision
      _f32ExposureTime      ("Exposure time (sec)", 1e-3, 9.99e-7, 3600),
      _f32CoolingTemp       ("Cooling Temp (C)",    25,   -300,  25),
      _enumFanMode          ("Fan Mode",            AndorConfigType::ENUM_FAN_FULL, lsEnumFanMode),
      _enumBaselineClamp    ("Baseline Clamp",      Enums::Disabled_Disable, Enums::Disabled_Names ),
      _enumHighCapacity     ("High Capacity",       Enums::Disabled_Disable, Enums::Disabled_Names ),
      _u8GainIndex          ("Gain Index",          0,    0,    5),
      _u16ReadoutSpeedIndex ("Readout Speed",       0,    0,    5),
      _boxTrigger           ("External Trigger",    false),
      _u16ExposureEventCode ("Exposure Event Code", 1,    1,    255),
      _u32NumDelayShots     ("Num Integration Shots",  1,    0,    0x7FFFFFFF)
    {
      pList.insert(&_uWidth);
      pList.insert(&_uHeight);
      pList.insert(&_uOrgX);
      pList.insert(&_uOrgY);
      pList.insert(&_uBinX);
      pList.insert(&_boxBinning);
      pList.insert(&_uBinY);
      pList.insert(&_f32ExposureTime);
      pList.insert(&_f32CoolingTemp);
      pList.insert(&_enumFanMode);
      pList.insert(&_enumBaselineClamp);
      pList.insert(&_enumHighCapacity);
      pList.insert(&_u8GainIndex);
      pList.insert(&_u16ReadoutSpeedIndex);
      pList.insert(&_boxTrigger);
      pList.insert(&_u16ExposureEventCode);
      pList.insert(&_u32NumDelayShots);
    }

  public:
    void flush ()
    {
      for(Parameter* p=pList.forward(); p!=pList.empty(); p=p->forward()) p->flush();
    }
    void update()
    {
      for(Parameter* p=pList.forward(); p!=pList.empty(); p=p->forward()) p->update();
    }
    void enable(bool l)
    {
      for(Parameter* p=pList.forward(); p!=pList.empty(); p=p->forward()) p->enable(l);
    }

  public:
    QLayout* initialize(QWidget* p) {
      QVBoxLayout* layout = new QVBoxLayout;
      layout->addLayout(_uWidth.initialize(p));
      layout->addLayout(_uHeight.initialize(p));
      layout->addLayout(_uOrgX.initialize(p));
      layout->addLayout(_uOrgY.initialize(p));
      layout->addLayout(_uBinX.initialize(p));
      layout->addLayout(_boxBinning.initialize(p));
      layout->addLayout(_concealerBinning.add(_uBinY.initialize(p)));
      layout->addLayout(_f32ExposureTime.initialize(p));
      layout->addLayout(_f32CoolingTemp.initialize(p));
      layout->addLayout(_enumFanMode.initialize(p));
      layout->addLayout(_enumBaselineClamp.initialize(p));
      layout->addLayout(_enumHighCapacity.initialize(p));
      layout->addLayout(_u8GainIndex.initialize(p));
      layout->addLayout(_u16ReadoutSpeedIndex.initialize(p));
      layout->addLayout(_boxTrigger.initialize(p));
      layout->addLayout(_concealerTrigger.add(_u16ExposureEventCode.initialize(p)));
      layout->addLayout(_concealerTrigger.add(_u32NumDelayShots.initialize(p)));
      if (Parameter::allowEdit()) {
        QObject::connect(_boxBinning._input, SIGNAL(toggled(bool)), &_concealerBinning, SLOT(hide(bool)));
        QObject::connect(_boxTrigger._input, SIGNAL(toggled(bool)), &_concealerTrigger, SLOT(hide(bool)));
      }
      return (layout);
    }

    int pull(void* from) {
      AndorConfigType& tc = *new(from) AndorConfigType;
      _uWidth               .value = tc.width   ();
      _uHeight              .value = tc.height  ();
      _uOrgX                .value = tc.orgX    ();
      _uOrgY                .value = tc.orgY    ();
      _uBinX                .value = tc.binX    ();
      _uBinY                .value = tc.binY    ();
      _f32ExposureTime      .value = tc.exposureTime();
      _f32CoolingTemp       .value = tc.coolingTemp ();
      _enumFanMode          .value = (AndorConfigType::EnumFanMode) tc.fanMode();
      _enumBaselineClamp    .value = (Enums::Disabled) tc.baselineClamp();
      _enumHighCapacity     .value = (Enums::Disabled) tc.highCapacity();
      _u8GainIndex          .value = tc.gainIndex();
      _u16ReadoutSpeedIndex .value = tc.readoutSpeedIndex();
      _u16ExposureEventCode .value = tc.exposureEventCode();
      _u32NumDelayShots     .value = tc.numDelayShots();
      _boxBinning           .value = (_uHeight.value == _uBinY.value);
      _boxTrigger           .value = (_u32NumDelayShots.value == 0);

      _concealerBinning.show(!_boxBinning.value);
      _concealerTrigger.show(!_boxTrigger.value);

      return tc._sizeof();
    }

    int push(void* to) {
      if (_boxBinning.value) {
        // full binning mode
        _uBinY.value = _uHeight.value;
      }
      if (_boxTrigger.value) {
        // external trigger mode
        _u32NumDelayShots.value = 0;
      }

      AndorConfigType& tc = *new(to) AndorConfigType(
        _uWidth               .value,
        _uHeight              .value,
        _uOrgX                .value,
        _uOrgY                .value,
        _uBinX                .value,
        _uBinY                .value,
        _f32ExposureTime      .value,
        _f32CoolingTemp       .value,
        _enumFanMode          .value,
        _enumBaselineClamp    .value,
        _enumHighCapacity     .value,
        _u8GainIndex          .value,
        _u16ReadoutSpeedIndex .value,
        _u16ExposureEventCode .value,
        _u32NumDelayShots     .value
      );
      return tc._sizeof();
    }

    int dataSize() const {
      return sizeof(AndorConfigType);
    }

  public:
    Pds::LinkedList<Parameter> pList;
    NumericInt<uint32_t>    _uWidth;
    NumericInt<uint32_t>    _uHeight;
    NumericInt<uint32_t>    _uOrgX;
    NumericInt<uint32_t>    _uOrgY;
    NumericInt<uint32_t>    _uBinX;
    CheckValue              _boxBinning;
    NumericInt<uint32_t>    _uBinY;
    NumericFloat<float>     _f32ExposureTime;
    NumericFloat<float>     _f32CoolingTemp;
    Enumerated<AndorConfigType::EnumFanMode>  _enumFanMode;
    Enumerated<Enums::Disabled>               _enumBaselineClamp;
    Enumerated<Enums::Disabled>               _enumHighCapacity;
    NumericInt<uint8_t>     _u8GainIndex;
    NumericInt<uint16_t>    _u16ReadoutSpeedIndex;
    CheckValue              _boxTrigger;
    NumericInt<uint16_t>    _u16ExposureEventCode;
    NumericInt<uint32_t>    _u32NumDelayShots;
    QtConcealer             _concealerBinning;
    QtConcealer             _concealerTrigger;
  };

  const char* AndorConfig::Private_Data::lsEnumFanMode[] = { "Full", "Low", "Off", "Off during Acq", NULL};

};

using namespace Pds_ConfigDb;

AndorConfig::AndorConfig() :
  Serializer("andor_Config"),
  _private_data( new Private_Data )
{
  pList.insert(_private_data);
}

int  AndorConfig::readParameters (void* from) {
  return _private_data->pull(from);
}

int  AndorConfig::writeParameters(void* to) {
  return _private_data->push(to);
}

int  AndorConfig::dataSize() const {
  return _private_data->dataSize();
}

#include "Parameters.icc"

