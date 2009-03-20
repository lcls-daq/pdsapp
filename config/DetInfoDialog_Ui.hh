#ifndef Pds_ConfigDb_DetInfoDialog_Ui_hh
#define Pds_ConfigDb_DetInfoDialog_Ui_hh

#include "pdsapp/config/Device.hh"

#include "pdsdata/xtc/DetInfo.hh"

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
    DetInfoDialog_Ui(QWidget*, const list<Pds::DetInfo>&);
    ~DetInfoDialog_Ui();
  public:
    const list<Pds::DetInfo>& src_list() const { return _list; }
  public slots:
    void add   (); 
    void remove();
  private:
    list<Pds::DetInfo> _list;
    QComboBox*     _detlist;
    QLineEdit*     _detedit;
    QComboBox*     _devlist;
    QLineEdit*     _devedit;
    QListWidget*   _srclist;
  };
};

#endif
