#ifndef Pds_ControlScan_hh
#define Pds_ControlScan_hh

#include <QtGui/QWidget>

class QCheckBox;
class QButtonGroup;
class QLineEdit;

namespace Pds_ConfigDb {

  class ControlScan : public QWidget {
    Q_OBJECT
  public:
    ControlScan();
    ~ControlScan();
  public slots:
    void create();
  private:
    QLineEdit* _steps       ;
    QLineEdit* _control_name;
    QLineEdit* _control_lo  ;
    QLineEdit* _control_hi  ;
    QLineEdit* _readback_name  ;
    QLineEdit* _readback_offset;
    QLineEdit* _readback_margin;
    QCheckBox* _settleB;
    QLineEdit* _settle_value ;
    QButtonGroup* _acqB;
    QLineEdit* _events_value ;
    QLineEdit* _time_value   ;
  };
};

#endif
