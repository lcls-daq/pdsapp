#ifndef Pds_EvrEventDesc_hh
#define Pds_EvrEventDesc_hh

#include <QtCore/QObject>

#include "pds/config/EvrConfigType.hh"
#include "pdsapp/config/Parameters.hh"
#include "pdsapp/config/ParameterSet.hh"

class QCheckBox;
class QComboBox;
class QGridLayout;
class QLabel;
class QStackedWidget;
class QWidget;

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
    void pull      (const EventCodeType& c);
    void push      (EventCodeType* c) const;
  public:
    bool enabled   () const;
    const char*    get_label() const;
    void set_enable(bool);
    void setGroupEnable(bool bEnableGroup);
  public slots:
    void enable    (bool);
    void update_p  ();
    void update_group(int);
  private:
    bool                         _enabled;
  public:
    QCheckBox*                   _enable;
  private:
    QComboBox*                   _type;
    QComboBox*                   _group;
    QStackedWidget*              _stack;
    TextParameter                _desc;
    NumericInt<unsigned>         _trans_delay;
    NumericInt<unsigned>         _trans_width;
    NumericInt<unsigned>         _latch_delay;
    NumericInt<unsigned>         _latch_release;
    bool                         _bEnableGroup;
  };
};

#endif    
