#ifndef PdsConfigDb_EvrPulseTable_V5_hh
#define PdsConfigDb_EvrPulseTable_V5_hh

#include "pdsapp/config/Parameters.hh"
#include "pdsdata/evr/ConfigV5.hh"

#include <QtCore/QObject>

class QButtonGroup;

namespace Pds_ConfigDb
{
  class Pulse_V5;
  class QrLabel;
  class EvrPulseTable_V5Q;

  class EvrPulseTable_V5 : public Parameter {
  public:
    enum    { MaxPulses = 12 };
    EvrPulseTable_V5(unsigned id);
    ~EvrPulseTable_V5();
  public:
    void     pull  (const Pds::EvrData::ConfigV5&);
    //  validate() updates pulses, outputs accessors
    bool validate(unsigned ncodes, 
                  const Pds::EvrData::EventCodeV5* codes,
                  int delay_offset,
                  unsigned, Pds::EvrData::PulseConfigV3*,
                  unsigned, Pds::EvrData::OutputMap*);

    unsigned npulses () const;
    unsigned noutputs() const;
  public:
    QLayout* initialize(QWidget*);
    void     flush     ();
    void     update    ();
    void     enable    (bool);
  public:
    void     update_enable    (int);
    void     update_output    (int);
  public:
    unsigned                   _id;
    QrLabel*                   _outputs[Pds::EvrData::ConfigV5::EvrOutputs];
    Pulse_V5*                  _pulses [MaxPulses];
    QButtonGroup*              _enable_group;
    QButtonGroup*              _outputs_group;
    Pds::LinkedList<Parameter> _pList;
    EvrPulseTable_V5Q*            _qlink;
    unsigned                   _npulses;
    unsigned                   _noutputs;
  };

  class EvrPulseTable_V5Q : public QObject {
    Q_OBJECT
  public:
    EvrPulseTable_V5Q(EvrPulseTable_V5&,
		   QWidget*);
  public slots:
    void     update_enable    (int);
    void     update_output    (int);
  private:
    EvrPulseTable_V5& _table;
  };
};

#endif
