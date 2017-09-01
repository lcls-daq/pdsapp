#define __STDC_LIMIT_MACROS

#include "JungfrauConfig.hh"

#include "pdsapp/config/Parameters.hh"
#include "pdsapp/config/ParameterSet.hh"
#include "pdsapp/config/QtConcealer.hh"
#include "pds/config/JungfrauConfigType.hh"
#include <QtGui/QVBoxLayout>
#include <QtGui/QLabel>

#include <new>

#include <stdint.h>
#include <float.h>
#include <stdio.h>


using namespace Pds_ConfigDb;

class Pds_ConfigDb::JungfrauConfig::Private_Data : public Parameter {
  static const char*  lsEnumGainMode[];
  static const char*  lsEnumSpeedMode[];
 public:
   Private_Data(bool expert_mode);
  ~Private_Data();

   QLayout* initialize( QWidget* p );
   int pull( void* from );
   int push( void* to );
   int dataSize() const
      { return sizeof(JungfrauConfigType); }
   void flush ()
      { for(Parameter* p=pList.forward(); p!=pList.empty(); p=p->forward()) p->flush(); }
   void update()
      { for(Parameter* p=pList.forward(); p!=pList.empty(); p=p->forward()) p->update(); }
    void enable(bool l)
      { for(Parameter* p=pList.forward(); p!=pList.empty(); p=p->forward()) p->enable(l); }

  Pds::LinkedList<Parameter> pList;
  bool _expert_mode;
  NumericInt<uint32_t> _numberOfModules;
  NumericInt<uint32_t> _numberOfRowsPerModule;
  NumericInt<uint32_t> _numberOfColumnsPerModule;
  NumericInt<uint32_t> _biasVoltage;
  Enumerated<JungfrauConfigType::GainMode> _gainMode;
  Enumerated<JungfrauConfigType::SpeedMode> _speedMode;
  NumericFloat<double> _triggerDelay;
  NumericFloat<double> _exposureTime;
  NumericFloat<double> _exposurePeriod;
  NumericInt<uint16_t> _vb_ds;
  NumericInt<uint16_t> _vb_comp;
  NumericInt<uint16_t> _vb_pixbuf;
  NumericInt<uint16_t> _vref_ds;
  NumericInt<uint16_t> _vref_comp;
  NumericInt<uint16_t> _vref_prech;
  NumericInt<uint16_t> _vin_com;
  NumericInt<uint16_t> _vdd_prot;
  QtConcealer          _concealerExpert;
};

const char* Pds_ConfigDb::JungfrauConfig::Private_Data::lsEnumGainMode[] = { "Normal", "FixedGain1", "FixedGain2", "ForcedGain1", "ForcedGain2", "HighGain0", NULL};
const char* Pds_ConfigDb::JungfrauConfig::Private_Data::lsEnumSpeedMode[] = { "Quarter", "Half", NULL};

Pds_ConfigDb::JungfrauConfig::Private_Data::Private_Data(bool expert_mode) :
  _expert_mode              (expert_mode),
  _numberOfModules          ("Number of Modules",         1,    1,      4),
  _numberOfRowsPerModule    ("Number of Rows",            512,  1,      512),
  _numberOfColumnsPerModule ("Number of Columns",         1024, 1,      1024),
  _biasVoltage              ("Bias Voltage (V)",          200,  0,      500),
  _gainMode                 ("Gain Mode",                 JungfrauConfigType::Normal, lsEnumGainMode),
  _speedMode                ("Speed Mode",                JungfrauConfigType::Quarter, lsEnumSpeedMode),
  _triggerDelay             ("Trigger Delay (s)",         0.000238, 0., 10.),
  _exposureTime             ("Exposure Time (s)",         0.000010, 0., 120.),
  _exposurePeriod           ("Exposure Period (s)",       0.005,    0., 120.),
  _vb_ds                    ("vb_ds",                     1000,     0,  4095),
  _vb_comp                  ("vb_comp",                   1220,     0,  4095),
  _vb_pixbuf                ("vb_pixbuf",                 750,      0,  4095),
  _vref_ds                  ("vref_ds",                   480,      0,  4095),
  _vref_comp                ("vref_comp",                 420,      0,  4095),
  _vref_prech               ("vref_prech",                1450,     0,  4095),
  _vin_com                  ("vin_com",                   1053,     0,  4095),
  _vdd_prot                 ("vdd_prot",                  3000,     0,  4095)
{
  pList.insert( &_numberOfModules );
  pList.insert( &_numberOfRowsPerModule );
  pList.insert( &_numberOfColumnsPerModule );
  pList.insert( &_biasVoltage );
  pList.insert( &_gainMode );
  pList.insert( &_speedMode );
  pList.insert( &_triggerDelay );
  pList.insert( &_exposureTime );
  pList.insert( &_exposurePeriod );
  pList.insert( &_vb_ds );
  pList.insert( &_vb_comp );
  pList.insert( &_vb_pixbuf );
  pList.insert( &_vref_ds );
  pList.insert( &_vref_comp );
  pList.insert( &_vref_prech );
  pList.insert( &_vin_com );
  pList.insert( &_vdd_prot );
}

Pds_ConfigDb::JungfrauConfig::Private_Data::~Private_Data() 
{}

QLayout* Pds_ConfigDb::JungfrauConfig::Private_Data::initialize(QWidget* p)
{
  QVBoxLayout* layout = new QVBoxLayout;
  { QVBoxLayout* m = new QVBoxLayout;
    m->addLayout(_concealerExpert.add(_numberOfModules          .initialize(p)));
    m->addLayout(_concealerExpert.add(_numberOfRowsPerModule    .initialize(p)));
    m->addLayout(_concealerExpert.add(_numberOfColumnsPerModule .initialize(p)));
    m->setSpacing(5);
    layout->addLayout(m); }
  layout->addStretch();
  { QVBoxLayout* t = new QVBoxLayout;
    t->addLayout(_biasVoltage              .initialize(p));
    t->addLayout(_gainMode                 .initialize(p));
    t->addLayout(_speedMode                .initialize(p));
    t->addLayout(_triggerDelay             .initialize(p));
    t->addLayout(_exposureTime             .initialize(p));
    t->addLayout(_exposurePeriod           .initialize(p));
    t->setSpacing(5);
    layout->addLayout(t); }
  layout->addStretch();
  { QVBoxLayout* d = new QVBoxLayout;
    d->addWidget(new QLabel("DAC registers: 12bit on 0 to 2.5V: "));
    d->addLayout(_vb_ds                   .initialize(p));
    d->addLayout(_vb_comp                 .initialize(p));
    d->addLayout(_vb_pixbuf               .initialize(p));
    d->addLayout(_vref_ds                 .initialize(p));
    d->addLayout(_vref_comp               .initialize(p));
    d->addLayout(_vref_prech              .initialize(p));
    d->addLayout(_vin_com                 .initialize(p));
    d->addLayout(_vdd_prot                .initialize(p));
    d->setSpacing(5);
    layout->addLayout(d); }
  layout->setSpacing(25);

  return layout;
}

int Pds_ConfigDb::JungfrauConfig::Private_Data::pull( void* from )
{
  JungfrauConfigType& cfg = * new (from) JungfrauConfigType;
  _numberOfModules.value          = cfg.numberOfModules();
  _numberOfRowsPerModule.value    = cfg.numberOfRowsPerModule();
  _numberOfColumnsPerModule.value = cfg.numberOfColumnsPerModule();
  _biasVoltage.value              = cfg.biasVoltage();
  _gainMode.value                 = (JungfrauConfigType::GainMode) cfg.gainMode();
  _speedMode.value                = (JungfrauConfigType::SpeedMode) cfg.speedMode();
  _triggerDelay.value             = cfg.triggerDelay();
  _exposureTime.value             = cfg.exposureTime();
  _exposurePeriod.value           = cfg.exposurePeriod();
  _vb_ds.value                    = cfg.vb_ds();
  _vb_comp.value                  = cfg.vb_comp();
  _vb_pixbuf.value                = cfg.vb_pixbuf();
  _vref_ds.value                  = cfg.vref_ds();
  _vref_comp.value                = cfg.vref_comp();
  _vref_prech.value               = cfg.vref_prech();
  _vin_com.value                  = cfg.vin_com();
  _vdd_prot.value                 = cfg.vdd_prot();

  _concealerExpert.show(_expert_mode);

  return sizeof(JungfrauConfigType);
}
  
int Pds_ConfigDb::JungfrauConfig::Private_Data::push(void* to)
{
  new (to) JungfrauConfigType(
    _numberOfModules.value,
    _numberOfRowsPerModule.value,
    _numberOfColumnsPerModule.value,
    _biasVoltage.value,
    _gainMode.value,
    _speedMode.value,
    _triggerDelay.value,
    _exposureTime.value,
    _exposurePeriod.value,
    _vb_ds.value,
    _vb_comp.value,
    _vb_pixbuf.value,
    _vref_ds.value,
    _vref_comp.value,
    _vref_prech.value,
    _vin_com.value,
    _vdd_prot.value
  );
  
  return sizeof(JungfrauConfigType);
}
 
Pds_ConfigDb::JungfrauConfig::JungfrauConfig(bool expert_mode) :
  Serializer("JungfrauConfig"),
  _private_data( new Private_Data(expert_mode) )
{
  pList.insert(_private_data);
}

int Pds_ConfigDb::JungfrauConfig::readParameters (void* from)
{
  return _private_data->pull(from);
}

int Pds_ConfigDb::JungfrauConfig::writeParameters(void* to)
{
  return _private_data->push(to);
}

int Pds_ConfigDb::JungfrauConfig::dataSize() const
{
  return _private_data->dataSize();
}

#include "Parameters.icc"
