#ifndef Pds_Runnable_hh
#define Pds_Runnable_hh

#include <QtGui/QGroupBox>

class QString;
class QLabel;

namespace Pds {
  class PartitionControl;

  class Runnable : public QGroupBox {
    Q_OBJECT
  public:
    Runnable(QWidget*,
	     PartitionControl&);
    ~Runnable();
  signals:
    void monitor_changed(const QString&);
  public slots:
    void update_monitor(const QString&);
  private:
    PartitionControl& _control;
    QLabel*           _monitor_display;
    QLabel*           _control_display;
  };
};

#endif
