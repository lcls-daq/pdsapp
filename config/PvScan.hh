#ifndef Pds_PvScan_hh
#define Pds_PvScan_hh

#include <QtGui/QWidget>

#include "pdsdata/psddl/control.ddl.h"

#include <list>

class QLineEdit;
class QCheckBox;
class QString;

namespace Pds { class ClockTime; };

namespace Pds_ConfigDb {
  class PvScan : public QWidget {
    Q_OBJECT
  public:
    PvScan(QWidget*);
    ~PvScan();
  public:
    int  write(unsigned step, unsigned nsteps, bool usePvs, const Pds::ClockTime&, char*) const;
    int  write(unsigned step, unsigned nsteps, bool usePvs, unsigned nevents     , char*) const;
    void read (const char*, int);
  private:
    void _fill_pvs(unsigned step, unsigned nsteps,
		   std::list<Pds::ControlData::PVControl>& controls,
		   std::list<Pds::ControlData::PVMonitor>& monitors) const;
  private:
    QLineEdit* _control_name;
    QLineEdit* _control_lo  ;
    QLineEdit* _control_hi  ;
    QLineEdit* _readback_name  ;
    QLineEdit* _readback_offset;
    QLineEdit* _readback_margin;
    QCheckBox* _settleB;
    QLineEdit* _settle_value ;
  };
};

#endif
