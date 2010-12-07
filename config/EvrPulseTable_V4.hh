#ifndef PdsConfigDb_EvrPulseTable_V4_hh
#define PdsConfigDb_EvrPulseTable_V4_hh

#include "pdsapp/config/Parameters.hh"

#include <QtCore/QObject>

class QButtonGroup;

namespace Pds_ConfigDb
{
  class EvrConfig_V4;
  class Pulse_V4;
  class QrLabel;
  class EvrPulseTable_V4Q;

  class EvrPulseTable_V4 : public Parameter {
  public:
    enum    { MaxEventCodes = 32 };
    enum    { MaxPulses = 12 };
    enum    { MaxOutputs = 10 };
    EvrPulseTable_V4(const EvrConfig_V4& c);
    ~EvrPulseTable_V4();
  public:
    void insert(Pds::LinkedList<Parameter>& pList);
    int  pull  (const void* from);
    int  push  (void* to) const;
    int  dataSize() const;
    bool validate();
  public:
    QLayout* initialize(QWidget* parent);
    void     flush     ();
    void     update    ();
    void     enable    (bool);
  public:
    void     update_enable    (int);
    void     update_terminator(int);
    void     update_output    (int);
    void     update_eventcode ();
  public:
    const EvrConfig_V4&        _cfg;
    QrLabel*                   _outputs[MaxOutputs];
    Pulse_V4*                  _pulses [MaxPulses];
    QButtonGroup*              _enable_group;
    QButtonGroup*              _terminator_group;
    QButtonGroup*              _outputs_group;
    Pds::LinkedList<Parameter> _pList;
    EvrPulseTable_V4Q*            _qlink;
  };

  class EvrPulseTable_V4Q : public QObject {
    Q_OBJECT
  public:
    EvrPulseTable_V4Q(EvrPulseTable_V4&,
		   QWidget*);
  public slots:
    void     update_enable    (int);
    void     update_terminator(int);
    void     update_output    (int);
    void     update_eventcode ();
  private:
    EvrPulseTable_V4& _table;
  };
};

#endif
