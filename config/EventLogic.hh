#ifndef Pds_EventLogic_hh
#define Pds_EventLogic_hh

#include <QtCore/QObject>

#include "pdsapp/config/Parameters.hh"
#include "pdsdata/psddl/timetool.ddl.h"

#include <vector>

class QGridLayout;
class QPushButton;

namespace Pds_ConfigDb {
  class EventLogic : public QObject, 
		     public Parameter {
    Q_OBJECT
  public:
    EventLogic(const char* name, 
	       bool lnot, 
	       unsigned code);
    ~EventLogic();
  public:
    QLayout* initialize(QWidget* p);
    void update();
    void flush ();
    void enable(bool l);
  public:
    void set(const ndarray<const Pds::TimeTool::EventLogic,1>&);
    ndarray<const Pds::TimeTool::EventLogic,1> get() const;
  public slots:
    void add_row   ();
    void remove_row();
  public:
    const char* _name;
    Enumerated<Enums::Polarity>_op0;
    std::vector< Enumerated<Pds::TimeTool::EventLogic::LogicOp>* > _op;
    std::vector< NumericInt<unsigned>* > _code;
    QGridLayout* _layout;
    QPushButton* _addB;
  };
};

#endif
