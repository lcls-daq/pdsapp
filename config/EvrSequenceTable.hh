#ifndef PdsConfigDb_EvrSequenceTable_hh
#define PdsConfigDb_EvrSequenceTable_hh

#include "pdsapp/config/Parameters.hh"

#include <QtCore/QObject>

class QButtonGroup;

namespace Pds_ConfigDb
{
  class EvrConfigP;
  class Pulse;
  class QrLabel;
  class EvrSequenceTableQ;

  class EvrSequenceTable : public Parameter {
  public:
    EvrSequenceTable(const EvrConfigP& c);
    ~EvrSequenceTable();
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
    
    const EvrConfigP&          _cfg;
    QrLabel*                   _outputs[MaxOutputs];
    Pulse*                     _pulses [MaxPulses];
    QButtonGroup*              _enable_group;
    QButtonGroup*              _terminator_group;
    QButtonGroup*              _outputs_group;
    Pds::LinkedList<Parameter> _pList;
    EvrSequenceTableQ*         _qlink;
  };

  class EvrSequenceTableQ : public QObject {
    Q_OBJECT
  public:
    EvrSequenceTableQ(EvrSequenceTable&,
		   QWidget*);
  public slots:
    void     update_enable    (int);
    void     update_terminator(int);
    void     update_output    (int);
    void     update_eventcode ();
  private:
    EvrSequenceTable& _table;
  };
};

#endif
