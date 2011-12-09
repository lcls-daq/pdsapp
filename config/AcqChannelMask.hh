#ifndef PdsConfigDb_AcqChannelMask
#define PdsConfigDb_AcqChannelMask

#include <QtCore/QObject>
#include "pdsapp/config/Parameters.hh"

class QCheckBox;
class QWidget;

namespace Pds_ConfigDb {
  class AcqChannelMask : public QObject,
                         public NumericInt<unsigned> {
    Q_OBJECT
  public:
    AcqChannelMask(const char* label, unsigned val);
    ~AcqChannelMask();
  public:
    QLayout* initialize(QWidget* parent);
    void     flush     ();
  public slots:
    void boxChanged(int);
    void numberChanged();
  private:
    enum { Channels=4, Modules=5 };
    QCheckBox* _box[Channels*Modules];
  };

};

#endif
