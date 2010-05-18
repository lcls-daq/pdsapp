#ifndef Pds_ParameterSet_hh
#define Pds_ParameterSet_hh

#include "pdsapp/config/Parameters.hh"

#include <QtCore/QObject>

class QComboBox;

namespace Pds_ConfigDb {
  class ParameterCount;
  class ParameterSetQ;

  class ParameterSet : public Parameter  {
  public:
    ParameterSet(const char* label, 
		 Pds::LinkedList<Parameter>* array,
		 ParameterCount& count);
    ~ParameterSet();

    QLayout* initialize(QWidget*);
    void     update();
    void     flush ();
    void     enable(bool);
  public:
    void launch(int);
    void membersChanged();
  public:
    Pds::LinkedList<Parameter>* _array;
    ParameterCount&             _count;
    QComboBox*                  _box;
    ParameterSetQ*              _qset;
  };

  class ParameterSetQ : public QObject {
    Q_OBJECT
  public:
    ParameterSetQ(ParameterSet&);
    ~ParameterSetQ();
  public slots:
    void launch(int);
    void membersChanged();
  public:
    ParameterSet& _pset;
  };
};

#endif
