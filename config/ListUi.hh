#ifndef Pds_ConfigDb_ListUi_hh
#define Pds_ConfigDb_ListUi_hh

#include <QtGui/QWidget>

#include "pdsapp/config/SerializerDictionary.hh"

#include <vector>
using std::vector;
#include <string>
using std::string;

class QListWidget;

namespace Pds_ConfigDb {

  class DbClient;

  class ListUi : public QWidget {
    Q_OBJECT
  public:
    ListUi(DbClient&);
    ~ListUi();
  public slots:    
    void update_device_list();
    void update_xtc_list();
    void view_xtc();
  private:
    SerializerDictionary _dict;
    DbClient&            _db;
    QListWidget* _keylist;
    QListWidget* _devlist;
    QListWidget* _xtclist;
    vector<uint64_t>    _devices;
    vector<Pds::TypeId> _types;
    unsigned         _lenbuff;
    char*            _xtcbuff;
  };
};

#endif
