#ifndef PdsConfigDb_EvrPulseTable_hh
#define PdsConfigDb_EvrPulseTable_hh

#include "pdsapp/config/Parameters.hh"

#include <QtCore/QObject>

class QButtonGroup;

namespace Pds_ConfigDb
{
  class EvrConfig;
  class Pulse;
  class QrLabel;
  class EvrPulseTableQ;

  class EvrPulseTable : public Parameter {
  public:
    enum    { MaxEventCodes = 32 };
    enum    { MaxPulses = 12 };
    enum    { MaxOutputs = 10 };
    EvrPulseTable(const EvrConfig& c);
    ~EvrPulseTable();
  public:
    void insert(Pds::LinkedList<Parameter>& pList);
    int  pull  (const void* from);
    int  push  (void* to) const;
    int dataSize() const;
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
    const EvrConfig&           _cfg;
    QrLabel*                   _outputs[MaxOutputs];
    Pulse*                     _pulses [MaxPulses];
    QButtonGroup*              _enable_group;
    QButtonGroup*              _terminator_group;
    QButtonGroup*              _outputs_group;
    Pds::LinkedList<Parameter> _pList;
    EvrPulseTableQ*            _qlink;
  };

  class EvrPulseTableQ : public QObject {
    Q_OBJECT
  public:
    EvrPulseTableQ(EvrPulseTable&,
		   QWidget*);
  public slots:
    void     update_enable    (int);
    void     update_terminator(int);
    void     update_output    (int);
    void     update_eventcode ();
  private:
    EvrPulseTable& _table;
  };
};

#endif
