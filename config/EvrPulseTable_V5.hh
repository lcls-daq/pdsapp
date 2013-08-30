#ifndef PdsConfigDb_EvrPulseTable_V5_hh
#define PdsConfigDb_EvrPulseTable_V5_hh

#include "pdsapp/config/EvrConfigType_V5.hh"
#include "pdsapp/config/Parameters.hh"

#include <QtCore/QObject>

class QButtonGroup;

namespace Pds_ConfigDb
{
  class QrLabel;

  namespace EvrConfig_V5 {

    class Pulse_V5;
    class EvrPulseTable_V5Q;

    class EvrPulseTable_V5 : public Parameter {
    public:
      enum    { MaxPulses = 12 };
      EvrPulseTable_V5(unsigned id);
      ~EvrPulseTable_V5();
    public:
      void     pull  (const EvrConfigType&);
      //  validate() updates pulses, outputs accessors
      bool validate(unsigned ncodes, 
                    const EventCodeType* codes,
                    int delay_offset,
                    unsigned, PulseType*,
                    unsigned, OutputMapType*);

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
      QrLabel*                   _outputs[EvrConfigType::EvrOutputs];
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
};

#endif
