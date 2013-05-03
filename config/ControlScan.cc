#include "pdsapp/config/ControlScan.hh"

#include "pdsapp/config/Experiment.hh"
#include "pdsapp/config/PdsDefs.hh"
#include "pdsapp/config/PvScan.hh"
#include "pdsapp/config/EvrScan.hh"
#include "pdsapp/config/XtcTable.hh"

#include "pdsdata/control/PVControl.hh"
#include "pdsdata/control/PVMonitor.hh"
#include "pdsdata/xtc/ClockTime.hh"

#include "pds/config/ControlConfigType.hh"
#include "pds/config/EvrConfigType.hh"

#include "pdsdata/xtc/Level.hh"
#include "pdsdata/xtc/DetInfo.hh"
#include "pdsdata/xtc/ProcInfo.hh"

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
#include <stdio.h>

using namespace Pds_ConfigDb;
using Pds::DetInfo;
using Pds::ProcInfo;

static const char* scan_file = "live_scan.xtc";

enum { Events, Duration };
enum { PvTab , TriggerTab };

ControlScan::ControlScan(QWidget* parent, Experiment& expt) :
  QWidget(0),
  _expt        (expt),
  _steps       (new QLineEdit),
  _acqB         (new QButtonGroup),
  _events_value (new QLineEdit),
  _time_value   (new QLineEdit),
  _buf_control  (new char[0x100000]),
  _buf_evr      (new char[0x100000])
{
  printf("ControlScan::create[%p]\n",this);

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
  delete[] _buf_control;
  delete[] _buf_evr;
}

void ControlScan::apply()
{
  write();
  emit reconfigure();
}

void ControlScan::set_run_type(const QString& runType)
{
  printf("ControlScan::set_run_type[%p] %s\n",this,qPrintable(runType));
  _run_type = std::string(qPrintable(runType));
  read(scan_file);
}

void ControlScan::write()
{
  char buff[128];

  XtcTable xtc(_expt.path().base());

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

    char* p = _buf_control;
    for(unsigned k=0; k<=steps; k++) {
      bool usePvs = (_tab->currentIndex() == PvTab);
      if (_acqB->checkedId()==Duration) {
	double s = _time_value->text().toDouble();
	Pds::ClockTime ctime(unsigned(s),unsigned(fmod(s,1.)*1.e9));
        int len = usePvs ? 
          _pv->write(k, steps, usePvs, ctime, p) :
          _evr->write_control(k, steps, ctime, p);
	fwrite(p, len, 1, output);
        p += len;
      }
      else {
	int len = usePvs ?
          _pv->write(k, steps, usePvs, _events_value->text().toInt(), p) :
          _evr->write_control(k, steps, _events_value->text().toInt(), p);
        fwrite(p, len, 1, output);
        p += len;
      }
    }
    fclose(output);

    xtc.update_xtc(PdsDefs::qtypeName(utype), scan_file);
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

    char* p = _buf_evr;
    for(unsigned k=0; k<=steps; k++) {
      int len = _evr->write(k,steps,p);
      fwrite(p, len, 1, output);
      p += len;
    }

    fclose(output);

    xtc.update_xtc(PdsDefs::qtypeName(utype), scan_file);
  }

  xtc.write(_expt.path().base());
}


void ControlScan::read(const char* ifile)
{
  printf("ControlScan::read %s\n",ifile);

  char namebuf[256];

  {
    //
    //  Fetch the last ControlConfig scan setting
    //
    UTypeName utype = PdsDefs::utypeName(PdsDefs::RunControl);
    string path = _expt.path().data_path("",utype);
    QString file = QString("%1/%2").arg(path.c_str()).arg(ifile);

    snprintf(namebuf,sizeof(namebuf),"%s",qPrintable(file));

    printf("ControlScan::read %s\n",namebuf);

    struct stat64 sstat;
    if (stat64(namebuf,&sstat)) {
      printf("ControlScan::read stat failed on controlconfig file %s\n",namebuf);

      // Create it
      printf("ControlScan::creating %s\n",namebuf);
      FILE* f = fopen(namebuf,"w");
      ControlConfigType* cfg = new (_buf_control)ControlConfigType(ControlConfigType::Default);
      fwrite(cfg, cfg->size(), 1, f);
      fclose(f);
      // retry stat()
      if (stat64(namebuf,&sstat)) {
        printf("ControlScan::read stat failed on controlconfig file %s\n",namebuf);
      }
    }

    FILE* f = fopen(namebuf,"r");

    int len = fread(_buf_control, 1, sstat.st_size, f);
    if (len != sstat.st_size) {
      printf("Read %d/%lld bytes from %s\n",len,sstat.st_size,namebuf);
    }
    else {
      const ControlConfigType& cfg = 
        *reinterpret_cast<const ControlConfigType*>(_buf_control);

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

      _pv->read(_buf_control, len);
    }

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
	sprintf(namebuf,"%s",qPrintable(file));

	printf("ControlScan::read %s\n",namebuf);

	struct stat64 sstat;
	if (stat64(namebuf,&sstat)) {
	  printf("ControlScan::read stat failed on evrconfig file %s\n",namebuf);
	  return;
	}

	FILE* f = fopen(namebuf,"r");

	int len = fread(_buf_evr, 1, sstat.st_size, f);
	if (len != sstat.st_size) {
	  printf("Read %d/%lld bytes from %s\n",len,sstat.st_size,namebuf);
	}
        else {
          _evr->read(_buf_evr, len);
        }
	fclose(f);
      }
    }
  }
}

int ControlScan::update_key()
{
  unsigned key = _expt.clone(_run_type);
  unsigned npts = _steps->text().toInt()+1;
  if (key) {
    const ControlConfigType& cfg = 
      *reinterpret_cast<const ControlConfigType*>(_buf_control);
    _expt.substitute(key, ProcInfo(Pds::Level::Control,0,0),
		     *PdsDefs::typeId(PdsDefs::RunControl), 
                     _buf_control, cfg.size()*npts);

    if (_tab->currentIndex() == TriggerTab) {
      const EvrConfigType& evr = 
        *reinterpret_cast<const EvrConfigType*>(_buf_evr);
      for(unsigned i=0; i<8; i++)
	_expt.substitute(key, DetInfo(0,DetInfo::NoDetector,0,DetInfo::Evr,i),
			 *PdsDefs::typeId(PdsDefs::Evr), 
			 _buf_evr, evr.size()*npts);
    }
  }

  return key;
}

bool ControlScan::pvscan() const { return _tab->currentIndex() == PvTab; }
