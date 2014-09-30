#ifndef Pds_ConfigDb_Restore_Ui_hh
#define Pds_ConfigDb_Restore_Ui_hh

#include <QtGui/QDialog>

#include "pdsapp/config/SerializerDictionary.hh"
#include "pds/config/ClientData.hh"

#include <list>
#include <string>

class QListWidget;

namespace Pds_ConfigDb {

  class DbClient;
  class Device;

  class Restore_Ui : public QDialog {
    Q_OBJECT
  public:
    Restore_Ui(QWidget*, DbClient&, 
	       const Device&, 
	       Pds::TypeId,
	       const std::string&);
    ~Restore_Ui();
  public slots:    
    void view_xtc();
  private:
    SerializerDictionary _dict;
    DbClient&            _db;
    std::string          _name;
    QListWidget*         _xtclist;
    std::list<XtcEntryT> _xtc;
    unsigned             _lenbuff;
    char*                _xtcbuff;
  };
};

#endif
