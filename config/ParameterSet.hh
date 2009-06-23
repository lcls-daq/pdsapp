#ifndef Pds_ParameterSet_hh
#define Pds_ParameterSet_hh

#include "pdsapp/config/Parameters.hh"

#include <QtCore/QObject>

class QComboBox;

namespace Pds_ConfigDb {

  class ParameterSet : public QObject, public Parameter  {
    Q_OBJECT
  public:
    ParameterSet(const char* label, 
		 Pds::LinkedList<Parameter>* array,
		 NumericInt<unsigned>& nmembers);
    ~ParameterSet();

    QLayout* initialize(QWidget*);
    void     update();
    void     flush ();
  public slots:
    void launch(int);
    void membersChanged();
  public:
    Pds::LinkedList<Parameter>* _array;
    NumericInt<unsigned>&       _nmembers;
    QComboBox*                  _box;
  };

};

#endif
