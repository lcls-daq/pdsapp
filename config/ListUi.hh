#ifndef Pds_ConfigDb_ListUi_hh
#define Pds_ConfigDb_ListUi_hh

#include <QtGui/QWidget>

#include "pdsapp/config/SerializerDictionary.hh"
#include "pdsapp/config/Path.hh"

#include <vector>
using std::vector;
#include <string>
using std::string;

class QListWidget;

namespace Pds_ConfigDb {

  class ListUi : public QWidget {
    Q_OBJECT
  public:
    ListUi(const Path&);
    ~ListUi();
  public slots:    
    void update_device_list();
    void update_xtc_list();
    void view_xtc();
  private:
    SerializerDictionary _dict;
    Path         _path;
    QListWidget* _keylist;
    QListWidget* _devlist;
    QListWidget* _xtclist;
    vector<string> _devices;
    vector<string> _types;
  };
};

#endif
