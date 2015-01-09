#ifndef Pds_ConfigDb_ROI_hh
#define Pds_ConfigDb_ROI_hh

#include "pdsapp/config/Parameters.hh"

class QStackedWidget;

namespace Pds_ConfigDb {
  class ROI : public Parameter {
  public:
    ROI(const char* label,
	const char* a,
	const char* b); 
  public:
    QLayout* initialize(QWidget* p);
    void update();
    void flush();
    void enable(bool l);
  public:
    QStackedWidget*      _sw;
    NumericInt<unsigned> _lo;
    NumericInt<unsigned> _hi;
  };
};

#endif
