#ifndef Pds_L0Select_hh
#define Pds_L0Select_hh

#include "pds/service/LinkedList.hh"
#include "pdsapp/config/Parameters.hh"
#include "pds/config/TriggerConfigType.hh"

#include <QtCore/QObject>
#include <QtGui/QTabWidget>

namespace Pds_ConfigDb {
  namespace Trigger { 
    class FixedRate;
    class ACRate; 
    class SeqSelect; 
    class CodeSelect; 
  }

  class L0Select : public QObject, public Parameter {
    Q_OBJECT
  public:
    L0Select();
    ~L0Select();
  public:
    void pull(const L0SelectType&);
    int  push(L0SelectType* to) const;
    bool validate();
  public:
    void insert(Pds::LinkedList<Parameter>& pList);
    QLayout* initialize(QWidget*);
    void update();
    void flush();
    void enable(bool v);
  private:
    Pds_ConfigDb::Trigger::FixedRate*  _fixedRate;
    Pds_ConfigDb::Trigger::ACRate*     _acRate;
    Pds_ConfigDb::Trigger::SeqSelect*  _seqSelect;
    Pds_ConfigDb::Trigger::CodeSelect* _codeSelect;
    QTabWidget* _tab;
  };
};

#endif
