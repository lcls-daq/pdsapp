#ifndef Pds_EvrEventDesc_hh
#define Pds_EvrEventDesc_hh

#include <QtCore/QObject>

#include "pdsapp/config/Parameters.hh"
#include "pdsapp/config/ParameterSet.hh"

class QCheckBox;
class QComboBox;
class QGridLayout;
class QLabel;
class QStackedWidget;
class QWidget;

namespace Pds { namespace EvrData { class EventCodeV5; } }

namespace Pds_ConfigDb {
  class EvrEventDesc : public QObject {
    Q_OBJECT
  public:
    enum { Disabled=0xff };
    EvrEventDesc();
  public:
    virtual QWidget* code_widget() = 0;
    virtual unsigned get_code() const = 0;
    virtual void     set_code(unsigned) = 0;
  public:   
    void initialize(QGridLayout* l, unsigned row);
    void update    ();
    void flush     ();
    void pull      (const Pds::EvrData::EventCodeV5& c);
    void push      (Pds::EvrData::EventCodeV5* c) const;
  public:
    bool enabled   () const;
    const char*    get_label() const;
    void set_enable(bool);
  public slots:
    void enable    (bool);
    void update_p  ();
  private:
    bool                         _enabled;
  public:
    QCheckBox*                   _enable;
  private:
    QLabel*                      _code;
    QComboBox*                   _type;
    QStackedWidget*              _stack;
    TextParameter                _desc;
    NumericInt<unsigned>         _trans_delay;
    NumericInt<unsigned>         _trans_width;
    NumericInt<unsigned>         _latch_delay;
    NumericInt<unsigned>         _latch_release;
  };
};

#endif    
