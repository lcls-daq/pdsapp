#include "pdsapp/config/PvScan.hh"

#include "pdsdata/xtc/ClockTime.hh"

#include "pds/config/ControlConfigType.hh"

#include <QtGui/QGridLayout>
#include <QtGui/QCheckBox>
#include <QtGui/QLabel>
#include <QtGui/QLineEdit>
#include <QtGui/QDoubleValidator>

#include <math.h>
#include <stdlib.h>

using namespace Pds_ConfigDb;

static void _parse_name(const QString& text,
			QString& base,
			int&     index)
{
  QRegExp exp("\\[[0-9]+\\]");
  int i = text.indexOf(exp);
  base = text.mid(0,i);
  if (i==-1)
    index = -1;
  else
    index = text.mid(i+1,exp.matchedLength()-2).toInt();
}

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

enum { Events, Duration };

PvScan::PvScan(QWidget* parent) :
  QWidget(0),
  _control_name(new QLineEdit),
  _control_lo  (new QLineEdit),
  _control_hi  (new QLineEdit),
  _readback_name  (new QLineEdit),
  _readback_offset(new QLineEdit),
  _readback_margin(new QLineEdit),
  _settleB      (new QCheckBox("Settle Time")),
  _settle_value (new QLineEdit)
{
  new QDoubleValidator(_control_lo);
  new QDoubleValidator(_control_hi);
  new QDoubleValidator(_readback_offset);
  new QDoubleValidator(_readback_margin);
  new QDoubleValidator(_settle_value);

  _control_name->setMaximumWidth(200);
  _control_lo  ->setMaximumWidth(60);
  _control_hi  ->setMaximumWidth(60);
  _readback_name->setMaximumWidth(200);
  _readback_offset->setMaximumWidth(60);
  _readback_margin->setMaximumWidth(60);
  _settle_value ->setMaximumWidth(60);

  QGridLayout* layout1 = new QGridLayout;
  layout1->addWidget(new QLabel("Control Channel"),0,0,::Qt::AlignRight);
  layout1->addWidget(new QLabel("Start")          ,0,2,::Qt::AlignRight);
  layout1->addWidget(new QLabel("Stop" )          ,0,4,::Qt::AlignRight);
  layout1->addWidget(_control_name,0,1,::Qt::AlignLeft);
  layout1->addWidget(_control_lo  ,0,3,::Qt::AlignLeft);
  layout1->addWidget(_control_hi  ,0,5,::Qt::AlignLeft);
  layout1->addWidget(new QLabel("Readback Channel"),1,0,::Qt::AlignRight);
  layout1->addWidget(new QLabel("Offset")          ,1,2,::Qt::AlignRight);
  layout1->addWidget(new QLabel("Margin")          ,1,4,::Qt::AlignRight);
  layout1->addWidget(_readback_name  ,1,1,::Qt::AlignLeft);
  layout1->addWidget(_readback_offset,1,3,::Qt::AlignLeft);
  layout1->addWidget(_readback_margin,1,5,::Qt::AlignLeft);
  layout1->addWidget(_settleB      ,2,0);
  layout1->addWidget(_settle_value ,2,1);
  layout1->addWidget(new QLabel("seconds"),2,2);
  setLayout(layout1);

  _settleB->setEnabled(false);
}

PvScan::~PvScan()
{
}

void PvScan::_fill_pvs(unsigned step, unsigned nsteps,
		       std::list<Pds::ControlData::PVControl>& controls,
		       std::list<Pds::ControlData::PVMonitor>& monitors) const
{
  controls.clear();
  monitors.clear();

  QString control_base;
  int     control_index;
  _parse_name(_control_name ->text(), control_base, control_index);

  QString readback_base;
  int     readback_index;
  _parse_name(_readback_name->text(), readback_base, readback_index);

  double control_v, control_step;
  _parse_range(_control_lo->text(), _control_hi->text(), nsteps,
	       control_v, control_step);

  control_v += double(step)*control_step;

  double readback_o = _readback_offset->text().toDouble();
  double readback_m = _readback_margin->text().toDouble();

  if (!control_base.isEmpty()) {
    if (control_index>=0)
      controls.push_back(Pds::ControlData::PVControl(qPrintable(control_base),
						     control_index, 
						     control_v));
    else
      controls.push_back(Pds::ControlData::PVControl(qPrintable(control_base),
						     control_v));
  }
  if (!readback_base.isEmpty()) {
    if (readback_index>=0)
      monitors.push_back(Pds::ControlData::PVMonitor(qPrintable(readback_base),
						     readback_index, 
						     control_v + readback_o - readback_m,
						     control_v + readback_o + readback_m));
    else
      monitors.push_back(Pds::ControlData::PVMonitor(qPrintable(readback_base),
						     control_v + readback_o - readback_m,
						     control_v + readback_o + readback_m));
  }
}

int PvScan::write(unsigned step, unsigned nsteps, bool usePvs, const Pds::ClockTime& ctime, char* buff) const
{
  std::list<Pds::ControlData::PVControl> controls;
  std::list<Pds::ControlData::PVMonitor> monitors;
  if (usePvs)
    _fill_pvs(step, nsteps, controls, monitors);

  Pds::ControlData::ConfigV1* c = 
    new (buff) Pds::ControlData::ConfigV1(controls, monitors, ctime);

  return c->size();
}


int PvScan::write(unsigned step, unsigned nsteps, bool usePvs, unsigned nevents, char* buff) const
{
  std::list<Pds::ControlData::PVControl> controls;
  std::list<Pds::ControlData::PVMonitor> monitors;
  if (usePvs)
    _fill_pvs(step, nsteps, controls, monitors);

  Pds::ControlData::ConfigV1* c = 
    new (buff) Pds::ControlData::ConfigV1(controls, monitors, nevents);

  return c->size();
}

void PvScan::read(const char* dbuf, int len)
{
  const Pds::ControlData::ConfigV1& cfg = 
    *reinterpret_cast<const Pds::ControlData::ConfigV1*>(dbuf);

  int npts = len/cfg.size();
  printf("cfg size %d/%d (%d)\n",cfg.size(),len,npts);

  const Pds::ControlData::ConfigV1& lst = 
    *reinterpret_cast<const Pds::ControlData::ConfigV1*>(dbuf+(npts-1)*cfg.size());

  if (cfg.npvControls()) {
    const Pds::ControlData::PVControl& ctl = cfg.pvControl(0);
    if (ctl.array())
      _control_name->setText(QString("%1[%2]").arg(ctl.name()).arg(ctl.index()));
    else
      _control_name->setText(QString(ctl.name()));

    _control_lo->setText(QString::number(cfg.pvControl(0).value()));
    _control_hi->setText(QString::number(lst.pvControl(0).value()));
  }

  if (cfg.npvMonitors()) {
    const Pds::ControlData::PVMonitor& mon = cfg.pvMonitor(0);
    if (mon.array())
      _readback_name->setText(QString("%1[%2]").arg(mon.name()).arg(mon.index()));
    else
      _readback_name->setText(QString(mon.name()));

    _readback_offset->setText(QString::number(0.5*(mon.loValue()+mon.hiValue())-
					      cfg.pvControl(0).value()));
    _readback_margin->setText(QString::number(0.5*(mon.hiValue()-mon.loValue())));
  }
}
