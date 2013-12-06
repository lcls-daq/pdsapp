#ifndef PdsConfigDb_EvsPulseTable_hh
#define PdsConfigDb_EvsPulseTable_hh

#include "pdsapp/config/Parameters.hh"
#include "pds/config/EvrConfigType.hh"
#include "pds/config/EvsConfigType.hh"

#include <QtCore/QObject>
#include <QtGui/QTabWidget>

class QButtonGroup;

namespace Pds_ConfigDb
{
  class EvrConfigP;
  class EvsPulse;
  class QrLabel;
  class EvsPulseTableQ;

  class EvsPulseTable : public Parameter {
  public:
    enum { MaxPulses  = 12 };
    enum { MaxOutputs = 13 };
    EvsPulseTable();
    ~EvsPulseTable();
  public:
    bool     pull  (const EvrConfigType& cfg);
    bool     pull  (const EvsConfigType& cfg);
    //  validate() updates pulses, outputs accessors
    bool validate(unsigned ncodes, 
                  const EvsCodeType* codes);

    unsigned                            npulses () const { return _npulses; }
    const PulseType*     pulses () const 
    { return reinterpret_cast<const PulseType*>(_pulse_buffer); }

    unsigned                            noutputs() const { return _noutputs; }
    const OutputMapType* outputs () const 
    { return reinterpret_cast<const OutputMapType*>(_output_buffer); }

  public:
    QLayout* initialize(QWidget*);
    void     flush     ();
    void     update    ();
    void     enable    (bool); // virtual function. Need to be defined
    void     setReadGroupEnable(bool bEnableReadGroup);
  public:
    void     update_enable    (int);
    void     update_output    (int);
  public:
    QrLabel*                   _outputs[MaxOutputs];
    EvsPulse*                  _pulses [MaxPulses];
    QButtonGroup*              _enable_group;
    QButtonGroup*              _outputs_group;    
    Pds::LinkedList<Parameter> _pList;
    EvsPulseTableQ*            _qlink;    
    char*          _pulse_buffer;
    char*          _output_buffer;
    unsigned       _npulses;
    unsigned       _noutputs;
  private:
    bool                       _bEnableReadGroup;
    QLabel*                    _pLabelGroup;
  };

  class EvsPulseTableQ : public QObject {
    Q_OBJECT
  public:
    EvsPulseTableQ(EvsPulseTable&,
       QWidget*);
  public slots:
    void     update_enable    (int);
    void     update_output    (int);
  private:
    EvsPulseTable& _table;
  };
} // namespace Pds_ConfigDb

#endif
