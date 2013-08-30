#ifndef PdsConfigDb_EvrPulseTable_V6_hh
#define PdsConfigDb_EvrPulseTable_V6_hh

#include "pdsapp/config/Parameters.hh"
#include "pdsapp/config/EvrConfigType_V6.hh"

#include <QtCore/QObject>

class QButtonGroup;

namespace Pds_ConfigDb {
  class QrLabel;
  namespace EvrConfig_V6 {
    class Pulse;
    class EvrPulseTableQ;

    class EvrPulseTable : public Parameter {
    public:
      enum { MaxPulses  = 12 };
      enum { MaxOutputs = 13 };
      EvrPulseTable(unsigned id);
      ~EvrPulseTable();
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
      QrLabel*                   _outputs[MaxOutputs];
      Pulse*                     _pulses [MaxPulses];
      QButtonGroup*              _enable_group;
      QButtonGroup*              _outputs_group;
      Pds::LinkedList<Parameter> _pList;
      EvrPulseTableQ*            _qlink;
      unsigned                   _npulses;
      unsigned                   _noutputs;
    };

    class EvrPulseTableQ : public QObject {
      Q_OBJECT
    public:
      EvrPulseTableQ(EvrPulseTable&,
                     QWidget*);
    public slots:
      void     update_enable    (int);
      void     update_output    (int);
    private:
      EvrPulseTable& _table;
    };
  };
};

#endif
