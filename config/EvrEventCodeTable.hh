#ifndef Pds_EvrEventCodeTable_hh
#define Pds_EvrEventCodeTable_hh

#include "pds/config/EvrConfigType.hh"
#include "pds/service/LinkedList.hh"
#include "pdsapp/config/Parameters.hh"
#include "pdsapp/config/EvrPulseTable.hh"

#include <QtCore/QObject>

class QLayout;
class QGridLayout;
class QWidget;

namespace Pds { 
  namespace EvrData { 
    class ConfigV7; 
    class ConfigV7; 
    class EventCodeV6; 
    class SequencerConfigV1; 
  }
}

namespace Pds_ConfigDb {

  class EvrSeqEventDesc;
  class EvrGlbEventDesc;  
  class EvrEventDefault;

  class EvrEventCodeTable : public QObject, public Parameter {
    Q_OBJECT
  public:
    enum { MaxCodes=16 };
    EvrEventCodeTable(EvrPulseTables* pPulseTables);
    ~EvrEventCodeTable();
  public:
    void pull(const EvrConfigType&);
    int  push(EventCodeType* to) const;
    bool validate();
    unsigned ncodes() const;
    bool     enableReadoutGroup() const;
    const EventCodeType* codes() const;
  public:
    void insert(Pds::LinkedList<Parameter>& pList);
    QLayout* initialize(QWidget*);
    void update();
    void flush();
    void enable(bool v);
  signals:
    void update_codes(bool);
  public:
    QStringList code_names() const; // list of code names
    unsigned    code_lookup(unsigned) const; // code value from list index
    unsigned    code_index (unsigned) const; // list index from code value
  public slots:
    void onEnableReadGroup(int iIndex);
  private:
    EvrPulseTables*      _pPulseTables;
    QGridLayout*         _elayout;
    EvrSeqEventDesc*     _seq_code;
    EvrGlbEventDesc*     _glb_code;
    EvrEventDefault*     _defaults;
    unsigned             _ncodes;
    QLabel*              _pLabelGroup1;
    QLabel*              _pLabelGroup2;
    QComboBox*           _cbEnableReadGroup;
    char*                _code_buffer;
  };
};

#endif
