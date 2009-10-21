#include "pdsapp/config/ControlScan.hh"

#include "pdsapp/config/Parameters.hh"
#include "pdsapp/config/Dialog.hh"
#include "pdsapp/config/Experiment.hh"
#include "pdsapp/config/PdsDefs.hh"

#include "pdsdata/control/PVControl.hh"
#include "pdsdata/control/PVMonitor.hh"
#include "pdsdata/xtc/ClockTime.hh"

#include "pds/config/ControlConfigType.hh"

#include <QtGui/QGroupBox>
#include <QtGui/QVBoxLayout>
#include <QtGui/QHBoxLayout>
#include <QtGui/QGridLayout>
#include <QtGui/QCheckBox>
#include <QtGui/QPushButton>
#include <QtGui/QRadioButton>
#include <QtGui/QButtonGroup>
#include <QtGui/QLabel>
#include <QtGui/QLineEdit>
#include <QtGui/QIntValidator>
#include <QtGui/QDoubleValidator>
#include <QtGui/QFileDialog>

#include <list>

#include <math.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

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

static const char* scan_file = "live_scan.xtc";

enum { Events, Duration };

ControlScan::ControlScan(QWidget* parent, Experiment& expt) :
  QWidget(0),
  _expt        (expt),
  _steps       (new QLineEdit),
  _control_name(new QLineEdit),
  _control_lo  (new QLineEdit),
  _control_hi  (new QLineEdit),
  _readback_name  (new QLineEdit),
  _readback_offset(new QLineEdit),
  _readback_margin(new QLineEdit),
  _settleB      (new QCheckBox("Settle Time")),
  _settle_value (new QLineEdit),
  _acqB         (new QButtonGroup),
  _events_value (new QLineEdit),
  _time_value   (new QLineEdit)
{
  new QIntValidator(0,0x7fffffff,_steps);
  new QDoubleValidator(_control_lo);
  new QDoubleValidator(_control_hi);
  new QDoubleValidator(_readback_offset);
  new QDoubleValidator(_readback_margin);
  new QDoubleValidator(_settle_value);
  new QDoubleValidator(_events_value);
  new QDoubleValidator(_time_value);

  _steps       ->setMaximumWidth(60);
  _control_name->setMaximumWidth(200);
  _control_lo  ->setMaximumWidth(60);
  _control_hi  ->setMaximumWidth(60);
  _readback_name->setMaximumWidth(200);
  _readback_offset->setMaximumWidth(60);
  _readback_margin->setMaximumWidth(60);
  _settle_value ->setMaximumWidth(60);
  _events_value ->setMaximumWidth(60);
  _time_value   ->setMaximumWidth(60);

  QRadioButton* eventsB = new QRadioButton("Events");
  QRadioButton* timeB   = new QRadioButton("Time");
  _acqB->addButton(eventsB,Events);
  _acqB->addButton(timeB  ,Duration);
  eventsB->setChecked(true);

  QPushButton* applyB = new QPushButton("OK");
  QPushButton* editB  = new QPushButton("Details");
  QPushButton* closeB = new QPushButton("Close");

  QVBoxLayout* layout = new QVBoxLayout;
  { QHBoxLayout* layout1 = new QHBoxLayout;
    layout1->addWidget(new QLabel("Steps"));
    layout1->addWidget(_steps);
    layout1->addStretch();
    layout->addLayout(layout1); }
  { QGroupBox* ca_box = new QGroupBox("Channel Access");
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
    ca_box->setLayout(layout1);
    layout->addWidget(ca_box); }
  { QGroupBox* acq_box = new QGroupBox("Acquisition / Step");
    QVBoxLayout* layout1 = new QVBoxLayout;
    { QHBoxLayout* layout2 = new QHBoxLayout;
      layout2->addWidget(eventsB);
      layout2->addWidget(_events_value);
      layout2->addStretch();
      layout1->addLayout(layout2); }
    { QHBoxLayout* layout2 = new QHBoxLayout;
      layout2->addWidget(timeB);
      layout2->addWidget(_time_value);
      layout2->addWidget(new QLabel("seconds"));
      layout2->addStretch();
      layout1->addLayout(layout2); }
    acq_box->setLayout(layout1);
    layout->addWidget(acq_box); }
  { QHBoxLayout* layout1 = new QHBoxLayout;
    layout1->addStretch();
    layout1->addWidget(applyB);
    layout1->addWidget(editB);
    layout1->addWidget(closeB);
    layout1->addStretch();
    layout->addLayout(layout1); }
  setLayout(layout);

  connect(applyB, SIGNAL(clicked()), this, SLOT(update ()));
  connect(editB , SIGNAL(clicked()), this, SLOT(details()));
  connect(closeB, SIGNAL(clicked()), this, SLOT(hide   ()));

  _settleB->setEnabled(false);

  read(scan_file);
}

ControlScan::~ControlScan()
{
}

void ControlScan::update()
{
  write();
  int key = update_key();
  emit created(key);
}

void ControlScan::set_run_type(const QString& runType)
{
  _run_type = std::string(qPrintable(runType));
}

void ControlScan::details() // modal dialog
{
  write();

  UTypeName utype = PdsDefs::utypeName(PdsDefs::RunControl);
  string path = _expt.path().data_path("",utype);
  QString file = QString("%1/%2").arg(path.c_str()).arg(scan_file);

  Parameter::allowEdit(true);
  Serializer* s = _dict.lookup(_controlConfigType);
  Dialog* d = new Dialog(this, *s, file);
  d->exec();
  if (d->result() == QDialog::Accepted) {
    int key = update_key();
    emit created(key);
  }

  delete d;
}

void ControlScan::write()
{
  UTypeName utype = PdsDefs::utypeName(PdsDefs::RunControl);
  string path = _expt.path().data_path("",utype);
  QString file = QString("%1/%2").arg(path.c_str()).arg(scan_file);

  if (file.isNull() || file.isEmpty())
    return;

  const int bufsize = 0x1000;
  char* buff = new char[bufsize];
  sprintf(buff,"%s",qPrintable(file));

  FILE* output = fopen(buff,"w");

  unsigned steps = _steps->text().toInt();

  QString control_base;
  int     control_index;
  _parse_name(_control_name ->text(), control_base, control_index);

  QString readback_base;
  int     readback_index;
  _parse_name(_readback_name->text(), readback_base, readback_index);

  std::list<Pds::ControlData::PVControl> controls;
  std::list<Pds::ControlData::PVMonitor> monitors;

  double control_v, control_step;
  _parse_range(_control_lo->text(), _control_hi->text(), steps,
	       control_v, control_step);

  double readback_o = _readback_offset->text().toDouble();
  double readback_m = _readback_margin->text().toDouble();

  for(unsigned k=0; k<=steps; k++) {
    controls.clear();
    monitors.clear();
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
    Pds::ControlData::ConfigV1* c;
    if (_acqB->checkedId()==Duration) {
      double s = _time_value->text().toDouble();
      Pds::ClockTime ctime(unsigned(s),unsigned(fmod(s,1.)*1.e9));
      c = new (buff) Pds::ControlData::ConfigV1(controls, monitors, ctime);
    }
    else
      c = new (buff) Pds::ControlData::ConfigV1(controls, monitors, _events_value->text().toInt());
    fwrite(buff, c->size(), 1, output);
    control_v += control_step;
  }
  fclose(output);
  delete[] buff;
}


void ControlScan::read(const char* ifile)
{
  UTypeName utype = PdsDefs::utypeName(PdsDefs::RunControl);
  string path = _expt.path().data_path("",utype);
  QString file = QString("%1/%2").arg(path.c_str()).arg(ifile);

  if (file.isNull() || file.isEmpty())
    return;

  const int bufsize = 0x1000;
  char* buff = new char[bufsize];
  sprintf(buff,"%s",qPrintable(file));

  struct stat sstat;
  if (stat(buff,&sstat))
    return;

  FILE* f = fopen(buff,"r");

  char* dbuf = new char[sstat.st_size];
  int len = fread(dbuf, 1, sstat.st_size, f);
  if (len != sstat.st_size) {
    printf("Read %d/%ld bytes from %s\n",len,sstat.st_size,buff);
    return;
  }

  const Pds::ControlData::ConfigV1& cfg = 
    *reinterpret_cast<const Pds::ControlData::ConfigV1*>(dbuf);

  int npts = len/cfg.size();
  printf("cfg size %d/%d (%d)\n",cfg.size(),len,npts);
  _steps->setText(QString::number(npts-1));

  if (cfg.uses_duration()) {
    _acqB->button(Duration)->setChecked(true);
    double s = double(cfg.duration().seconds()) +
      1.e-9 *  double(cfg.duration().nanoseconds());
    _time_value->setText(QString::number(s));
  }
  else {
    _acqB->button(Events  )->setChecked(true);
    _events_value->setText(QString::number(cfg.events()));
  }

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
  
  delete[] dbuf;
  delete[] buff;
  fclose(f);
}


int ControlScan::update_key()
{
  static const char* dev_name = "Control";
  static const char* cfg_name = "SCAN";

  Pds_ConfigDb::UTypeName utype = PdsDefs::utypeName(PdsDefs::RunControl);

  _expt.table().set_entry(_run_type, FileEntry(dev_name, cfg_name));
  _expt.device(dev_name)->table().set_entry(cfg_name, FileEntry(utype, scan_file));

  _expt.update_key(*_expt.table().get_top_entry(_run_type));

  int key = strtoul(_expt.table().get_top_entry(_run_type)->key().c_str(),NULL,16);

  _expt.read();

  return key;
}
