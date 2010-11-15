#include "pdsapp/config/ControlScan.hh"

#include "pdsapp/config/Experiment.hh"
#include "pdsapp/config/PdsDefs.hh"
#include "pdsapp/config/PvScan.hh"
#include "pdsapp/config/EvrScan.hh"

#include "pdsdata/control/PVControl.hh"
#include "pdsdata/control/PVMonitor.hh"
#include "pdsdata/xtc/ClockTime.hh"

#include "pds/config/ControlConfigType.hh"
#include "pds/config/EvrConfigType.hh"

#include <QtGui/QTabWidget>
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

#include <list>

#include <math.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

using namespace Pds_ConfigDb;

static const char* scan_file = "live_scan.xtc";

enum { Events, Duration };
enum { PvTab , TriggerTab };

ControlScan::ControlScan(QWidget* parent, Experiment& expt) :
  QWidget(0),
  _expt        (expt),
  _steps       (new QLineEdit),
  _acqB         (new QButtonGroup),
  _events_value (new QLineEdit),
  _time_value   (new QLineEdit)
{
  new QIntValidator(0,0x7fffffff,_steps);
  new QDoubleValidator(_events_value);
  new QDoubleValidator(_time_value);

  _steps       ->setMaximumWidth(60);
  _events_value->setMaximumWidth(60);
  _time_value  ->setMaximumWidth(60);

  QRadioButton* eventsB = new QRadioButton("Events");
  QRadioButton* timeB   = new QRadioButton("Time");
  _acqB->addButton(eventsB,Events);
  _acqB->addButton(timeB  ,Duration);
  eventsB->setChecked(true);

  QPushButton* applyB = new QPushButton("Apply");
  //  QPushButton* editB  = new QPushButton("Details");
  QPushButton* closeB = new QPushButton("Close");

  QVBoxLayout* layout = new QVBoxLayout;
  { QHBoxLayout* layout1 = new QHBoxLayout;
    layout1->addWidget(new QLabel("Steps"));
    layout1->addWidget(_steps);
    layout1->addStretch();
    layout->addLayout(layout1); }
  { _tab = new QTabWidget(0);
    _tab->insertTab(PvTab     , _pv  = new PvScan (this), "EPICS PV");
    _tab->insertTab(TriggerTab, _evr = new EvrScan(this), "DAQ Trigger");
    layout->addWidget(_tab); }
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
    //    layout1->addWidget(editB);
    layout1->addWidget(closeB);
    layout1->addStretch();
    layout->addLayout(layout1); }
  setLayout(layout);

  connect(applyB, SIGNAL(clicked()), this, SLOT(apply  ()));
  //  connect(editB , SIGNAL(clicked()), this, SLOT(details()));
  connect(closeB, SIGNAL(clicked()), this, SIGNAL(deactivate()));

  read(scan_file);
}

ControlScan::~ControlScan()
{
}

void ControlScan::apply()
{
  write();
  emit reconfigure();
}

void ControlScan::set_run_type(const QString& runType)
{
  printf("ControlScan::set_run_type %s\n",qPrintable(runType));
  _run_type = std::string(qPrintable(runType));
  read(scan_file);
}

void ControlScan::write()
{
  const int bufsize = 0x1000;
  char* buff = new char[bufsize];

  //
  //  Create Control Config xtc file (always)
  //
  {
    UTypeName utype = PdsDefs::utypeName(PdsDefs::RunControl);
    string path = _expt.path().data_path("",utype);
    QString file = QString("%1/%2").arg(path.c_str()).arg(scan_file);

    if (file.isNull() || file.isEmpty())
      return;

    sprintf(buff,"%s",qPrintable(file));

    FILE* output = fopen(buff,"w");

    unsigned steps = _steps->text().toInt();

    for(unsigned k=0; k<=steps; k++) {
      bool usePvs = (_tab->currentIndex() == PvTab);
      if (_acqB->checkedId()==Duration) {
	double s = _time_value->text().toDouble();
	Pds::ClockTime ctime(unsigned(s),unsigned(fmod(s,1.)*1.e9));
	fwrite(buff, _pv->write(k, steps, usePvs, ctime, buff), 1, output);
      }
      else
	fwrite(buff, _pv->write(k, steps, usePvs, _events_value->text().toInt(), buff), 1, output);
    }
    fclose(output);
  }
  //
  //  Create EVR Config xtc file
  //
  if (_tab->currentIndex() == TriggerTab) {
    UTypeName utype = PdsDefs::utypeName(PdsDefs::Evr);
    string path = _expt.path().data_path("",utype);
    QString file = QString("%1/%2").arg(path.c_str()).arg(scan_file);

    if (file.isNull() || file.isEmpty())
      return;

    sprintf(buff,"%s",qPrintable(file));

    FILE* output = fopen(buff,"w");

    unsigned steps = _steps->text().toInt();

    for(unsigned k=0; k<=steps; k++)
      fwrite(buff, _evr->write(k,steps,buff), 1, output);

    fclose(output);
  }

  delete[] buff;
}


void ControlScan::read(const char* ifile)
{
  printf("ControlScan::read %s\n",ifile);

  const int bufsize = 0x1000;
  char* buff = new char[bufsize];

  {
    //
    //  Fetch the last ControlConfig scan setting
    //
    UTypeName utype = PdsDefs::utypeName(PdsDefs::RunControl);
    string path = _expt.path().data_path("",utype);
    QString file = QString("%1/%2").arg(path.c_str()).arg(ifile);

    sprintf(buff,"%s",qPrintable(file));

    printf("ControlScan::read %s\n",buff);

    struct stat sstat;
    if (stat(buff,&sstat)) {
      printf("ControlScan::read stat failed on controlconfig file %s\n",buff);
      return;
    }

    FILE* f = fopen(buff,"r");

    char* dbuf = new char[sstat.st_size];
    int len = fread(dbuf, 1, sstat.st_size, f);
    if (len != sstat.st_size) {
      printf("Read %d/%ld bytes from %s\n",len,sstat.st_size,buff);
    }
    else {
      const ControlConfigType& cfg = 
        *reinterpret_cast<const ControlConfigType*>(dbuf);

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

      _pv->read(dbuf, len);
    }
    delete[] dbuf;

    fclose(f);
  }

  { 
    //
    //  Fetch the current EVR configuration (not last scan setting)
    //
    UTypeName utype = PdsDefs::utypeName(PdsDefs::Evr);
    const TableEntry* entry = _expt.table().get_top_entry(_run_type);
    if (!entry)
      printf("ControlScan no EVR entry for run type %s\n",_run_type.c_str());
    else {
      string path = _expt.path().key_path(entry->key());
      Pds::DetInfo info(0,Pds::DetInfo::NoDetector,0,Pds::DetInfo::Evr,0);
      QString file = QString("%1/%2/%3")
	.arg(path.c_str())
	.arg(info.phy(),8,16,QChar('0'))
	.arg(_evrConfigType.value(),8,16,QChar('0'));

      if (file.isNull() || file.isEmpty()) {
	printf("No EVR configuration found for run type %s\n",_run_type.c_str());
      }
      else {
	sprintf(buff,"%s",qPrintable(file));

	printf("ControlScan::read %s\n",buff);

	struct stat sstat;
	if (stat(buff,&sstat)) {
	  printf("ControlScan::read stat failed on evrconfig file %s\n",buff);
	  return;
	}

	FILE* f = fopen(buff,"r");

	char* dbuf = new char[sstat.st_size];
	int len = fread(dbuf, 1, sstat.st_size, f);
	if (len != sstat.st_size) {
	  printf("Read %d/%ld bytes from %s\n",len,sstat.st_size,buff);
	}
        else {
          _evr->read(dbuf, len);
        }
	delete[] dbuf;
	fclose(f);
      }
    }
  }
  
  delete[] buff;
}

int ControlScan::update_key()
{
  static const char* cfg_name = "SCAN";

  {
    const char* dev_name = "Control";
    UTypeName utype = PdsDefs::utypeName(PdsDefs::RunControl);
    _expt.table().set_entry(_run_type, FileEntry(dev_name, cfg_name));
    _expt.device(dev_name)->table().set_entry(cfg_name, FileEntry(utype, scan_file));
  }

  if (_tab->currentIndex() == TriggerTab) {
    const char* dev_name = "EVR";
    UTypeName utype = PdsDefs::utypeName(PdsDefs::Evr);
    _expt.table().set_entry(_run_type, FileEntry(dev_name, cfg_name));
    _expt.device(dev_name)->table().set_entry(cfg_name, FileEntry(utype, scan_file));
  }

  _expt.update_key(*_expt.table().get_top_entry(_run_type));

  int key = strtoul(_expt.table().get_top_entry(_run_type)->key().c_str(),NULL,16);

  _expt.read();

  return key;
}

