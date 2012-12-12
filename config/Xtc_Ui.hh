#ifndef Pds_Xtc_Ui_hh
#define Pds_Xtc_Ui_hh

#include <QtGui/QWidget>

#include "pdsapp/config/SerializerDictionary.hh"

#include <string>
using std::string;

class QLabel;
class QListWidget;
class QListWidgetItem;

namespace Pds { class Dgram; class TypeId; }

namespace Pds_ConfigDb {

  class Serializer;

  class Xtc_Ui : public QWidget {
    Q_OBJECT
  public:
    Xtc_Ui(QWidget*);
    ~Xtc_Ui();
  public slots:
    void update_device_list();
    void update_component_list();
    void change_component     ();
    void set_file (QString);
  public:
    Serializer& lookup(const Pds::TypeId&);
  signals:
    void changed();
  private:
    char*                _dgram_buffer;
    Pds::Dgram*          _dgram;
    SerializerDictionary _dict;
    QLabel*      _runInfo;
    QListWidget* _devlist;
    QListWidget* _cmplist;
  };
};

#endif


        
        
