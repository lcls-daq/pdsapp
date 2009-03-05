#ifndef Pds_CfgParameterSet_hh
#define Pds_CfgParameterSet_hh

#include "ConfigTC_Parameters.hh"

#include <QtGui/QComboBox>

namespace ConfigGui {

  class SubDialog;

  class ParameterSet : public QComboBox, public Parameter  {
    Q_OBJECT
  public:
    ParameterSet(const char* label, 
		 Pds::LinkedList<Parameter>* array,
		 NumericInt<unsigned>& nmembers);
    ~ParameterSet();

    QLayout* initialize(QWidget*);
    void     update();
    void     flush();
  public slots:
    void launch(int);
    void membersChanged();
  public:
    Pds::LinkedList<Parameter>* _array;
    NumericInt<unsigned>&       _nmembers;
  };

};

#endif
