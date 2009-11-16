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
  public:
    void appendText(const QString&);
  signals:
    void appended(QString);
  };
};

Q_DECLARE_METATYPE(QTextCursor)

#endif
