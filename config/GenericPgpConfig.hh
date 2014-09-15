#ifndef Pds_GenericPgpConfig_hh
#define Pds_GenericPgpConfig_hh

#include "pdsapp/config/Serializer.hh"
#include "pdsapp/config/Parameters.hh"

#include <QtCore/QObject>

class QBoxLayout;
class QComboBox;
class QWidget;

namespace Pds_ConfigDb {

  class GenericPgpDisplay : public QObject {
    Q_OBJECT
  public:
    GenericPgpDisplay();
    ~GenericPgpDisplay();
  public:
    Serializer& s();
  public:
    int  read (void*);
    void setup(QWidget*,QBoxLayout*,QComboBox*);
  public slots:
    void changed(int);
  private:
    Serializer* _s;
    QWidget*    _p;
    QBoxLayout* _l;
  };

  class GenericPgpConfig : public Serializer {
  public:
    GenericPgpConfig();
    ~GenericPgpConfig();
  public:
    int  readParameters (void* from);
    int  writeParameters(void* to);
    int  dataSize       () const;
  private:
    class Private_Data;
    Private_Data* _private;
  };

};

#endif
