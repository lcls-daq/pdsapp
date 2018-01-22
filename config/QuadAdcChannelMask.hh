#ifndef PdsConfigDb_QuadAdcChannelMask
#define PdsConfigDb_QuadAdcChannelMask

#include <QtCore/QObject>
#include "pdsapp/config/Parameters.hh"

class QCheckBox;
class QButtonGroup;
class QWidget;

namespace Pds_ConfigDb {
  class QuadAdcChannelMask : public QObject,
                         public NumericInt<unsigned> {
    Q_OBJECT
  public:
    QuadAdcChannelMask(const char* label, unsigned val);
    ~QuadAdcChannelMask();
  public:
    QLayout* initialize(QWidget* parent);
    void     flush     ();
    void     setinterleave(int);
    int      interleave() const;
    void     setsamplerate(double);
    double   samplerate() const;
  public slots:
    void boxChanged(int);
    void modeChanged(int);
    void numberChanged();
  private:
    enum { Channels=4, Modules=1 };
    QCheckBox* _box[Channels*Modules];
    QButtonGroup* _group;
    QComboBox*   _combobox;
    QComboBox*   _rateBox;
  };

};

#endif
