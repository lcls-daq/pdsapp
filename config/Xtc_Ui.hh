#ifndef Pds_Xtc_Ui_hh
#define Pds_Xtc_Ui_hh

#include "pdsapp/config/SerializerDictionary.hh"

#include <QtGui/QDialog>

#include <string>
using std::string;

class QListWidget;
class QListWidgetItem;

namespace Pds { class Dgram; class TypeId; }

namespace Pds_ConfigDb {

  class Serializer;

  class Xtc_Ui : public QDialog {
    Q_OBJECT
  public:
    Xtc_Ui(QWidget*, Pds::Dgram&);
  public slots:
    void update_device_list();
    void update_component_list();
    void change_component     ();
  public:
    Serializer& lookup(const Pds::TypeId&);
  signals:
    void changed();
  private:
    Pds::Dgram&          _dgram;
    SerializerDictionary _dict;
    QListWidget* _devlist;
    QListWidget* _cmplist;
  };
};

#endif


        
        
