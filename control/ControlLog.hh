#ifndef Pds_ControlLog_hh
#define Pds_ControlLog_hh

#include <QtGui/QTextEdit>

namespace Pds {
  class ControlLog : public QTextEdit {
    Q_OBJECT
  public:
    ControlLog();
    ~ControlLog();
  };
};

#endif
