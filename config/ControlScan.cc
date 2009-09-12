#include "pdsapp/config/ControlScan.hh"

#include "pdsdata/control/PVControl.hh"
#include "pdsdata/control/PVMonitor.hh"
#include "pdsdata/control/ConfigV1.hh"
#include "pdsdata/xtc/ClockTime.hh"

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

using namespace Pds_ConfigDb;

static void _parse_name(const QString& text,
			QString& base,
			int&     index)
{
  QRegExp exp("\\[[0-9]+\\]");
  int i = exp.indexIn(text);
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

ControlScan::ControlScan() : 
  QWidget(0),
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
  new QIntValidator(_steps);
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
  _acqB->addButton(eventsB);
  _acqB->addButton(timeB  );
  eventsB->setChecked(true);

  QPushButton* saveB = new QPushButton("Save");

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
  { QGroupBox* acq_box = new QGroupBox("Acquisition");
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
    layout1->addWidget(saveB);
    layout1->addStretch();
    layout->addLayout(layout1); }
  setLayout(layout);

  connect(saveB, SIGNAL(clicked()), this, SLOT(create()));

  _settleB->setEnabled(false);
}

ControlScan::~ControlScan()
{
}

void ControlScan::create()
{
  QString file = QFileDialog::getSaveFileName(this,"File to write to:",".", "*.xtc");
  if (file.isNull() || file.isEmpty())
    return;

  const int bufsize = 0x1000;
  char* buff = new char[bufsize];
  sprintf(buff,"%s",qPrintable(file));

  FILE* output = fopen(buff,"w");
  int steps = _steps->text().toInt();

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
    if (control_index>=0)
      controls.push_back(Pds::ControlData::PVControl(qPrintable(control_base),
						     control_index, 
						     control_v));
    else
      controls.push_back(Pds::ControlData::PVControl(qPrintable(control_base),
						     control_v));
    if (readback_index>=0)
      monitors.push_back(Pds::ControlData::PVMonitor(qPrintable(readback_base),
						     readback_index, 
						     control_v + readback_o - readback_m,
						     control_v + readback_o + readback_m));
    else
      monitors.push_back(Pds::ControlData::PVMonitor(qPrintable(readback_base),
						     control_v + readback_o - readback_m,
						     control_v + readback_o + readback_m));
    Pds::ControlData::ConfigV1* c;
    if (_acqB->checkedId()==0) {
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
}

