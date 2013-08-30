#ifndef PdsConfigDb_EvrPulseTable_hh
#define PdsConfigDb_EvrPulseTable_hh

#include "pdsapp/config/Parameters.hh"
#include "pds/config/EvrConfigType.hh"

#include <QtCore/QObject>
#include <QtGui/QTabWidget>

class QButtonGroup;

namespace Pds_ConfigDb
{
  class EvrConfigP;
  class Pulse;
  class QrLabel;
  class EvrPulseTableQ;

  class EvrPulseTable : public Parameter {
  public:
    enum { MaxPulses  = 12 };
    enum { MaxOutputs = 13 };
    EvrPulseTable(unsigned id);
    ~EvrPulseTable();
  public:
    bool     pull  (const EvrConfigType& cfg);
    //  validate() updates pulses, outputs accessors
    bool validate(unsigned ncodes, 
                  const EventCodeType* codes,
                  //int delay_offset,
                  unsigned, PulseType*,
                  unsigned, OutputMapType*);

    unsigned npulses () const;
    unsigned noutputs() const;
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
    unsigned                   _id;
    QrLabel*                   _outputs[MaxOutputs];
    Pulse*                     _pulses [MaxPulses];
    QButtonGroup*              _enable_group;
    QButtonGroup*              _outputs_group;    
    Pds::LinkedList<Parameter> _pList;
    EvrPulseTableQ*            _qlink;    
    unsigned                   _npulses;
    unsigned                   _noutputs;
  private:
    bool                       _bEnableReadGroup;
    QLabel*                    _pLabelGroup;
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
  
  
  class EvrPulseTables : public Parameter {
  public:
    static const unsigned MaxEVRs = 8;
    EvrPulseTables();
    ~EvrPulseTables();    
  public:
    QLayout* initialize(QWidget*);    
    void     flush     () { for(unsigned i=0; i<_nevr; i++) _evr[i]->flush (); }
    void     update    () { for(unsigned i=0; i<_nevr; i++) _evr[i]->update(); }
    void     enable    (bool) {}
    void     setReadGroupEnable(bool bEnableReadGroup);
  public:
    void     pull    (const EvrConfigType& tc);
    bool     validate(unsigned ncodes,
                      const EventCodeType* codes);
    unsigned                            npulses () const { return _npulses; }
    const PulseType*     pulses () const 
    { return reinterpret_cast<const PulseType*>(_pulse_buffer); }

    unsigned                            noutputs() const { return _noutputs; }
    const OutputMapType* outputs () const 
    { return reinterpret_cast<const OutputMapType*>(_output_buffer); }

  private:
    EvrPulseTable* _evr[MaxEVRs];
    unsigned       _nevr;
    char*          _pulse_buffer;
    char*          _output_buffer;
    unsigned       _npulses;
    unsigned       _noutputs;
    QTabWidget*    _tab;
  };
  
} // namespace Pds_ConfigDb

#endif
