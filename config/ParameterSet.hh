#ifndef Pds_ParameterSet_hh
#define Pds_ParameterSet_hh

#include "pdsapp/config/Parameters.hh"

#include <QtCore/QObject>

class QComboBox;

namespace Pds_ConfigDb {
  class ParameterCount;
  class ParameterSet : public QObject, public Parameter  {
    Q_OBJECT
  public:
    ParameterSet(const char* label, 
		 Pds::LinkedList<Parameter>* array,
		 ParameterCount& count);
    ~ParameterSet();

    QLayout* initialize(QWidget*);
    void     update();
    void     flush ();
  public slots:
    void launch(int);
    void membersChanged();
  public:
    Pds::LinkedList<Parameter>* _array;
    ParameterCount&             _count;
    QComboBox*                  _box;
  };

};

#endif
