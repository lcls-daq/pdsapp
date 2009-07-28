#ifndef Pds_PVDisplay_hh
#define Pds_PVDisplay_hh

#include "pdsapp/control/PVRunnable.hh"

#include <QtGui/QGroupBox>

class QString;
class QPushButton;

namespace Pds {
  class QualifiedControl;

  class PVDisplay : public QGroupBox, public PVRunnable {
    Q_OBJECT
  public:
    PVDisplay(QWidget*,
	      QualifiedControl&);
    ~PVDisplay();
  public:
    void runnable_change(bool);
  signals:
    void control_changed(bool);
  public slots:
    void update_control(bool);
  private:
    QualifiedControl& _control;
    QPushButton*      _control_display;
    QPalette*         _green;
    QPalette*         _red;
  };
};

#endif
