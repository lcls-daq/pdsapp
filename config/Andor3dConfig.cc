#include "pdsapp/config/Andor3dConfig.hh"

#include "pdsapp/config/Parameters.hh"
#include "pdsapp/config/QtConcealer.hh"
#include "pds/config/Andor3dConfigType.hh"
#include <QtGui/QCheckBox>
#include <QtGui/QVBoxLayout>

#include <new>

namespace Pds_ConfigDb {

  // these must end in NULL
  static const char* gain_to_name[] = { "1x", "2x", "4x", NULL };
  static const char* readoutSpeed_to_name[] = { "3.000000 MHz", "1.000000 MHz", "0.050000 MHz", NULL };

  class Andor3dConfig::Private_Data : public Parameter {
  static const char*  lsEnumFanMode[];
  public:
    Private_Data() :
      _uWidth               ("Width",               16,   1,    4152),
      _uHeight              ("Height",              16,   1,    4128),
      _uNumSensors          ("Num Sensors",         2,    1,    10),
      _uOrgX                ("Orgin X",             0,    0,    4151),
      _uOrgY                ("Orgin Y",             0,    0,    4127),
      _uBinX                ("Binning X",           1,    1,    2048),
      _boxBinning           ("Full Binning Mode",   false),
      _uBinY                ("Binning Y",           1,    1,    2048),
      // Note: Here the min exposure time need to set 9.99e-7 to allow user to input 1e-6, due to floating points imprecision
      _f32ExposureTime      ("Exposure time (sec)", 1e-3, 9.99e-7, 3600),
      _f32CoolingTemp       ("Cooling Temp (C)",    25,   -300,  25),
      _enumFanMode          ("Fan Mode",            Andor3dConfigType::ENUM_FAN_FULL, lsEnumFanMode),
      _enumBaselineClamp    ("Baseline Clamp",      Enums::Disabled_Disable, Enums::Disabled_Names ),
      _enumHighCapacity     ("High Capacity",       Enums::Disabled_Disable, Enums::Disabled_Names ),
      _u8GainIndex          ("Gain",                0 /* 1x */, gain_to_name),
      _u16ReadoutSpeedIndex ("Readout Speed",       0 /* 3 MHz */, readoutSpeed_to_name),
      _boxTrigger           ("External Trigger",    false),
      _u16ExposureEventCode ("Exposure Event Code", 1,    1,    255),
      // Note: the start exposure/acq delay between the CCDs must be between 0.01 and 30 seconds
      _u32ExposureStartDelay("Exposure Start Delay",12500,10000,0x1C9C380),
      _u32NumDelayShots     ("Num Integration Shots",  1,    0,    0x7FFFFFFF)
    {
      pList.insert(&_uWidth);
      pList.insert(&_uHeight);
      pList.insert(&_uNumSensors);
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
      pList.insert(&_u32ExposureStartDelay);
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
      layout->addLayout(_uNumSensors.initialize(p));
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
      layout->addLayout(_concealerTrigger.add(_u32ExposureStartDelay.initialize(p)));
      layout->addLayout(_concealerTrigger.add(_u32NumDelayShots.initialize(p)));
      if (Parameter::allowEdit()) {
        QObject::connect(_boxBinning._input, SIGNAL(toggled(bool)), &_concealerBinning, SLOT(hide(bool)));
        QObject::connect(_boxTrigger._input, SIGNAL(toggled(bool)), &_concealerTrigger, SLOT(hide(bool)));
      }
      return (layout);
    }

    int pull(void* from) {
      Andor3dConfigType& tc = *new(from) Andor3dConfigType;
      _uWidth               .value = tc.width       ();
      _uHeight              .value = tc.height      ();
      _uNumSensors          .value = tc.numSensors  ();
      _uOrgX                .value = tc.orgX        ();
      _uOrgY                .value = tc.orgY        ();
      _uBinX                .value = tc.binX        ();
      _uBinY                .value = tc.binY        ();
      _f32ExposureTime      .value = tc.exposureTime();
      _f32CoolingTemp       .value = tc.coolingTemp ();
      _enumFanMode          .value = (Andor3dConfigType::EnumFanMode) tc.fanMode();
      _enumBaselineClamp    .value = (Enums::Disabled) tc.baselineClamp();
      _enumHighCapacity     .value = (Enums::Disabled) tc.highCapacity();
      _u8GainIndex          .value = tc.gainIndex();
      _u16ReadoutSpeedIndex .value = tc.readoutSpeedIndex();
      _u16ExposureEventCode .value = tc.exposureEventCode();
      _u32ExposureStartDelay.value = tc.exposureStartDelay();
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

      Andor3dConfigType& tc = *new(to) Andor3dConfigType(
        _uWidth               .value,
        _uHeight              .value,
        _uNumSensors          .value,
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
        _u32ExposureStartDelay.value,
        _u32NumDelayShots     .value
      );
      return tc._sizeof();
    }

    int dataSize() const {
      return sizeof(Andor3dConfigType);
    }

  public:
    Pds::LinkedList<Parameter> pList;
    NumericInt<uint32_t>    _uWidth;
    NumericInt<uint32_t>    _uHeight;
    NumericInt<uint32_t>    _uNumSensors;
    NumericInt<uint32_t>    _uOrgX;
    NumericInt<uint32_t>    _uOrgY;
    NumericInt<uint32_t>    _uBinX;
    CheckValue              _boxBinning;
    NumericInt<uint32_t>    _uBinY;
    NumericFloat<float>     _f32ExposureTime;
    NumericFloat<float>     _f32CoolingTemp;
    Enumerated<Andor3dConfigType::EnumFanMode>  _enumFanMode;
    Enumerated<Enums::Disabled>               _enumBaselineClamp;
    Enumerated<Enums::Disabled>               _enumHighCapacity;
    Enumerated<uint8_t>     _u8GainIndex;
    Enumerated<uint16_t>    _u16ReadoutSpeedIndex;
    CheckValue              _boxTrigger;
    NumericInt<uint16_t>    _u16ExposureEventCode;
    NumericInt<uint32_t>    _u32ExposureStartDelay;
    NumericInt<uint32_t>    _u32NumDelayShots;
    QtConcealer             _concealerBinning;
    QtConcealer             _concealerTrigger;
  };

  const char* Andor3dConfig::Private_Data::lsEnumFanMode[] = { "Full", "Low", "Off", "Off during Acq", NULL};

};

using namespace Pds_ConfigDb;

Andor3dConfig::Andor3dConfig() :
  Serializer("andor3d_Config"),
  _private_data( new Private_Data )
{
  pList.insert(_private_data);
}

int  Andor3dConfig::readParameters (void* from) {
  return _private_data->pull(from);
}

int  Andor3dConfig::writeParameters(void* to) {
  return _private_data->push(to);
}

int  Andor3dConfig::dataSize() const {
  return _private_data->dataSize();
}

#include "Parameters.icc"

