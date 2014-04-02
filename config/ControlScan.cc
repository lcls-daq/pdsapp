#include "pdsapp/config/ControlScan.hh"

#include "pdsapp/config/Experiment.hh"
#include "pds/config/PdsDefs.hh"
#include "pdsapp/config/PvScan.hh"
#include "pdsapp/config/EvrScan.hh"

#include "pdsdata/psddl/control.ddl.h"
#include "pdsdata/xtc/ClockTime.hh"

#include "pds/config/ControlConfigType.hh"
#include "pds/config/EvrConfigType.hh"

#include "pdsdata/xtc/Level.hh"
#include "pdsdata/xtc/DetInfo.hh"
#include "pdsdata/xtc/ProcInfo.hh"

#include "pds/config/DbClient.hh"

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
  _run_type = std::string(qPrintable(runType));
  read(scan_file);
}

void ControlScan::write()
{
  //
  //  Create Control Config xtc file (always)
  //
  {
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
        p += len;
      }
      else {
	int len = usePvs ?
          _pv->write(k, steps, usePvs, _events_value->text().toInt(), p) :
          _evr->write_control(k, steps, _events_value->text().toInt(), p);
        p += len;
      }
    }

    XtcEntry x;
    x.type_id = *PdsDefs::typeId(PdsDefs::RunControl);
    x.name    = scan_file;
    
    DbClient& db = _expt.path();
    db.begin();
    db.setXTC(x,_buf_control,p-_buf_control);
    db.commit();
  }
  //
  //  Create EVR Config xtc file
  //
  if (_tab->currentIndex() == TriggerTab) {

    unsigned steps = _steps->text().toInt();

    char* p = _buf_evr;
    for(unsigned k=0; k<=steps; k++) {
      int len = _evr->write(k,steps,p);
      p += len;
    }

    XtcEntry x;
    x.type_id = *PdsDefs::typeId(PdsDefs::Evr);
    x.name    = scan_file;
    
    DbClient& db = _expt.path();
    db.begin();
    db.setXTC(x,_buf_evr,p-_buf_evr);
    db.commit();
  }
}


void ControlScan::read(const char* ifile)
{
  DbClient& db = _expt.path();

  {
    //
    //  Fetch the last ControlConfig scan setting
    //
    XtcEntry x;
    x.type_id = *PdsDefs::typeId(PdsDefs::RunControl);
    x.name    = ifile;

    db.begin();
    int sz = db.getXTC(x);
    if (sz<=0) {
      db.abort();
      printf("ControlScan::read getXTC failed on %s\n",ifile);

      // Create it
      printf("ControlScan::creating %s\n",ifile);
      ControlConfigType* cfg = new (_buf_control)ControlConfigType(1,0,0,0,0,0,0,0,0,0,0);

      db.begin();
      sz = cfg->_sizeof();
      int len = db.setXTC(x,_buf_control,sz);
      if (len!=sz) {
        db.abort();
        printf("ControlScan::read setXTC failed %s [%d/%d]\n",ifile,len,sz);
      }
      else
        db.commit();
    }
    else
      db.commit();

    db.begin();
    int len = db.getXTC(x,_buf_control,sz);
    if (len != sz) {
      db.abort();
      printf("Read %d/%d bytes from %s\n",len,sz,x.name.c_str());
    }
    else {
      db.commit();

      const ControlConfigType& cfg = 
        *reinterpret_cast<const ControlConfigType*>(_buf_control);

      int npts = len/cfg._sizeof();
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
  }

  { 
    //
    //  Fetch the current EVR configuration (not last scan setting)
    //
    const TableEntry* entry = _expt.table().get_top_entry(_run_type);
    if (!entry)
      printf("ControlScan no EVR entry for run type %s\n",_run_type.c_str());
    else {
      Pds::DetInfo info(0,Pds::DetInfo::NoDetector,0,Pds::DetInfo::Evr,0);

      db.begin();
      std::list<KeyEntry> entries = 
        db.getKey(strtoul(entry->key().c_str(),NULL,16));
      db.commit();

      for(std::list<KeyEntry>::const_iterator it=entries.begin();
          it!=entries.end(); it++)
        if (DeviceEntry(it->source) == info &&
	    it->xtc.type_id.value() == _evrConfigType.value()) {
          db.begin();
          int sz = db.getXTC(it->xtc);
          if (sz<=0) {
            db.abort();
            printf("ControlScan no EVR entry for run type %s\n",
                   _run_type.c_str());
          }
          else {
            db.commit();

            db.begin();
            int len = db.getXTC(it->xtc, _buf_evr, sz);
            if (len != sz) {
              db.abort();
              printf("Read %d/%d bytes from %s\n",len,sz,it->xtc.name.c_str());
            }
            else {
              db.commit();
              _evr->read(_buf_evr, len);
            }
          }
          break;
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
                     _buf_control, cfg._sizeof()*npts);

    if (_tab->currentIndex() == TriggerTab) {
      const EvrConfigType& evr = 
        *reinterpret_cast<const EvrConfigType*>(_buf_evr);
      for(unsigned i=0; i<8; i++)
	_expt.substitute(key, DetInfo(0,DetInfo::NoDetector,0,DetInfo::Evr,i),
			 *PdsDefs::typeId(PdsDefs::Evr), 
			 _buf_evr, Pds::EvrConfig::size(evr)*npts);
    }
  }

  return key;
}

bool ControlScan::pvscan() const { return _tab->currentIndex() == PvTab; }
