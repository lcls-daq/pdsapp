#ifndef Pds_SubDialog_hh
#define Pds_SubDialog_hh

#include "pds/service/LinkedList.hh"

#include <QtGui/QDialog>

class QComboBox;

namespace Pds_ConfigDb {

  class Parameter;

  class SubDialog : public QDialog {
    Q_OBJECT
  public:
    SubDialog(QWidget* parent,
	      Pds::LinkedList<Parameter>& pList, QWidget* t = 0);
    ~SubDialog();
  public slots:
    void _return();
  private:
    Pds::LinkedList<Parameter>& _pList;
  };

};

#endif
