#ifndef Pds_ControlLog_hh
#define Pds_ControlLog_hh

#include <QtGui/QTextEdit>
#include <QtGui/QTextCursor>

namespace Pds {
  class ControlLog : public QTextEdit {
    Q_OBJECT
  public:
    ControlLog();
    ~ControlLog();
  };
};

Q_DECLARE_METATYPE(QTextCursor)

#endif
