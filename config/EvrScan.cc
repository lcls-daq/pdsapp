#include "pdsapp/config/EvrScan.hh"

#include "pdsapp/config/EventcodeTiming.hh"
#include "pds/config/EvrConfigType.hh"
#include "pds/config/ControlConfigType.hh"
#include "pdsdata/psddl/control.ddl.h"
#include "pdsdata/psddl/evr.ddl.h"

#include <QtGui/QGridLayout>
#include <QtGui/QLabel>
#include <QtGui/QComboBox>
#include <QtGui/QLineEdit>
#include <QtGui/QDoubleValidator>

#include <list>

#include <math.h>
#include <stdlib.h>
#include <stdio.h>

using namespace Pds_ConfigDb;

static void _parse_range(const QString& lo,
       const QString& hi,
       int steps,
       double& v0,
       double& dv)
{
  double x0 = lo.toDouble();
  double x1 = hi.toDouble();
  dv = (x1-x0)/double(steps);
  v0 = x0;
}

EvrScan::EvrScan(QWidget* parent) :
  QWidget  (0),
  _pulse_id(new QComboBox),
  _width   (new QLineEdit),
  _delay_lo(new QLineEdit),
  _delay_hi(new QLineEdit),
  _buff    (0)
{
  new QDoubleValidator(_width);
  new QDoubleValidator(_delay_lo);
  new QDoubleValidator(_delay_hi);

  _width   ->setMaximumWidth(120);
  _delay_lo->setMaximumWidth(120);
  _delay_hi->setMaximumWidth(120);

  QGridLayout* layout1 = new QGridLayout;
  layout1->addWidget(new QLabel("Pulse"),0,0,::Qt::AlignRight);
  layout1->addWidget(new QLabel("Width [s]"),0,2,::Qt::AlignRight);
  layout1->addWidget(new QLabel("Delay Begin [s]"),1,0,::Qt::AlignRight);
  layout1->addWidget(new QLabel("Delay End   [s]"),1,2,::Qt::AlignRight);
  layout1->addWidget(_pulse_id,0,1,::Qt::AlignLeft);
  layout1->addWidget(_width   ,0,3,::Qt::AlignLeft);
  layout1->addWidget(_delay_lo,1,1,::Qt::AlignLeft);
  layout1->addWidget(_delay_hi,1,3,::Qt::AlignLeft);
  setLayout(layout1);

  connect(_pulse_id, SIGNAL(currentIndexChanged(int)), this, SLOT(set_pulse(int)));
}

EvrScan::~EvrScan()
{
  if (_buff)
    delete[] _buff;
}

int EvrScan::write_control(unsigned step, unsigned nsteps, const Pds::ClockTime& ctime, char* buff) const
{
  std::list<Pds::ControlData::PVControl> controls;
  std::list<Pds::ControlData::PVMonitor> monitors;
  std::list<Pds::ControlData::PVLabel  > labels;

  QString control_base = QString("EVR:P%1:DELAY").arg(_pulse_id->currentIndex());

  double control_v, control_step;
  _parse_range(_delay_lo->text(), _delay_hi->text(), nsteps,
         control_v, control_step);
  control_v += double(step)*control_step;

  controls.push_back(Pds::ControlData::PVControl(qPrintable(control_base),
                                                 Pds::ControlData::PVControl::NoArray,
                                                 control_v));

  ControlConfigType* c = Pds::ControlConfig::_new(buff, controls, monitors, labels, ctime);

  return c->_sizeof();
}

int EvrScan::write_control(unsigned step, unsigned nsteps, unsigned nevents, char* buff) const
{
  std::list<Pds::ControlData::PVControl> controls;
  std::list<Pds::ControlData::PVMonitor> monitors;
  std::list<Pds::ControlData::PVLabel  > labels;

  QString control_base = QString("EVR:P%1:DELAY").arg(_pulse_id->currentIndex());

  double control_v, control_step;
  _parse_range(_delay_lo->text(), _delay_hi->text(), nsteps,
         control_v, control_step);
  control_v += double(step)*control_step;

  controls.push_back(Pds::ControlData::PVControl(qPrintable(control_base),
                                                 Pds::ControlData::PVControl::NoArray,
                                                 control_v));

  ControlConfigType* c = Pds::ControlConfig::_new(buff, controls, monitors, labels, nevents);

  return c->_sizeof();
}


int EvrScan::write(unsigned step, unsigned nsteps, char* buff) const
{
  double control_v, control_step;
  _parse_range(_delay_lo->text(), _delay_hi->text(), nsteps,
         control_v, control_step);
  control_v += double(step)*control_step;

  const EvrConfigType& cfg = *reinterpret_cast<const EvrConfigType*>(_buff);

  //  change the pulse configuration
  const Pds::EvrData::PulseConfigV3& pulse = cfg.pulses()[_pulse_id->currentIndex()];

  int offs = 0;
  for(unsigned i=0; i<cfg.neventcodes(); i++)
    if (cfg.eventcodes()[i].maskTrigger() & (1<<pulse.pulseId())) {
      offs = int(Pds_ConfigDb::EventcodeTiming::timeslot(cfg.eventcodes()[i].code())) 
	-    int(Pds_ConfigDb::EventcodeTiming::timeslot(140));
      break;
    }

  uint32_t delay = uint32_t(control_v * 119.e6 / double(pulse.prescale())) - offs;
  uint32_t width = uint32_t(_width->text().toDouble() * 119.e6 / double(pulse.prescale()));

  new (const_cast<Pds::EvrData::PulseConfigV3*>(&pulse))
    Pds::EvrData::PulseConfigV3(pulse.pulseId(),
        pulse.polarity(),
        pulse.prescale(),
        delay,
        width);
  
  EvrConfigType* c   = new(buff) EvrConfigType(cfg.neventcodes(), 
                                               cfg.npulses    (), 
                                               cfg.noutputs   (), 
                                               &cfg.eventcodes()[0],
                                               &cfg.pulses()[0],
                                               &cfg.output_maps()[0],
                                               cfg.seq_config ());
  
  
  return Pds::EvrConfig::size(*c);
}


void EvrScan::read(const char* dbuf, int len)
{
  if (_buff)
    delete[] _buff;

  _buff = new char[len];

  memcpy(_buff, dbuf, len);

  const EvrConfigType& cfg = *reinterpret_cast<const EvrConfigType*>(_buff);

  disconnect(_pulse_id, SIGNAL(activated(int)), this, SLOT(set_pulse(int)));

  _pulse_id->clear();

  for(unsigned i=0; i<cfg.npulses(); i++)
    _pulse_id->addItem(QString::number(i));

  connect(_pulse_id, SIGNAL(activated(int)), this, SLOT(set_pulse(int)));

  _pulse_id->setCurrentIndex(0);
  printf("EvrScan::read npulses %u\n",cfg.npulses());
}


void EvrScan::set_pulse(int index)
{
  const EvrConfigType& cfg = *reinterpret_cast<const EvrConfigType*>(_buff);
  const Pds::EvrData::PulseConfigV3& pulse = cfg.pulses()[index];
  printf("EvrScan::set_pulse %d\n", index);

  int offs = 0;
  for(unsigned i=0; i<cfg.neventcodes(); i++)
    if (cfg.eventcodes()[i].maskTrigger() & (1<<index)) {
      offs = int(Pds_ConfigDb::EventcodeTiming::timeslot(cfg.eventcodes()[i].code())) 
	-    int(Pds_ConfigDb::EventcodeTiming::timeslot(140));
      break;
    }

  double scale = double(pulse.prescale())/119.e6;
  _width   ->setText(QString::number(double(pulse.width())*scale));
  _delay_lo->setText(QString::number(double(pulse.delay()+offs)*scale));
  _delay_hi->setText(QString::number(double(pulse.delay()+offs)*scale));
}
