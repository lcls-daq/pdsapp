#ifndef Pds_EvrScan_hh
#define Pds_EvrScan_hh

#include <QtGui/QWidget>

class QComboBox;
class QLineEdit;

namespace Pds_ConfigDb {
  class EvrScan : public QWidget {
    Q_OBJECT
  public:
    EvrScan(QWidget*);
    ~EvrScan();
  public:
    int  write(unsigned step, unsigned nsteps, char*) const;
    void read (const char*, int);
  public slots:
    void set_pulse(int);
  private:
    QComboBox* _pulse_id  ;
    QLineEdit* _width     ;
    QLineEdit* _delay_lo  ;
    QLineEdit* _delay_hi  ;
    char*      _buff      ;
  };
};

#endif
