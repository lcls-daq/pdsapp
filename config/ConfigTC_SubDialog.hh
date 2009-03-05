#ifndef Pds_CfgSubDialog_hh
#define Pds_CfgSubDialog_hh

#include "pds/service/LinkedList.hh"

#include <QtGui/QDialog>

class QComboBox;

namespace ConfigGui {

  class Parameter;

  class SubDialog : public QDialog {
    Q_OBJECT
  public:
    SubDialog(QWidget* parent,
	      Pds::LinkedList<Parameter>& pList);
    ~SubDialog();
  public slots:
    void _return();
  private:
    Pds::LinkedList<Parameter>& _pList;
  };

};

#endif
