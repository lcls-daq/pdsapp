#ifndef Pds_ConfigDb_DetInfoDialog_Ui_hh
#define Pds_ConfigDb_DetInfoDialog_Ui_hh

#include "pdsapp/config/Device.hh"

#include "pdsdata/xtc/Src.hh"

#include <QtCore/QObject>
#include <QtGui/QDialog>

#include <list>
using std::list;

class QComboBox;
class QLineEdit;
class QListWidget;

namespace Pds_ConfigDb {

  class DetInfoDialog_Ui : public QDialog {
    Q_OBJECT
  public:
    DetInfoDialog_Ui(QWidget*, const list<Pds::Src>&);
    ~DetInfoDialog_Ui();
  public:
    const list<Pds::Src>& src_list() const { return _list; }
  public slots:
    void add    (); 
    void addproc(); 
    void remove ();
  private:
    void reconstitute_srclist();
  private:
    list<Pds::Src> _list;
    QComboBox*     _detlist;
    QLineEdit*     _detedit;
    QComboBox*     _devlist;
    QLineEdit*     _devedit;
    QComboBox*     _proclist;
    QListWidget*   _srclist;
  };
};

#endif
