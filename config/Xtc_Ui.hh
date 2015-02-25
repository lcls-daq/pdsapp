#ifndef Pds_Xtc_Ui_hh
#define Pds_Xtc_Ui_hh

#include <QtGui/QWidget>

#include "pdsapp/config/SerializerDictionary.hh"

#include <string>
using std::string;

class QLabel;
class QListWidget;
class QListWidgetItem;

namespace Pds { 
  class Dgram; 
  class TypeId; 
  class XtcFileIterator; 
}

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
    void prev_cycle();
    void next_cycle();
  public:
    Serializer& lookup(const Pds::TypeId&);
  signals:
    void changed();
    void set_cycle(int);
  private:
    char*                _cfgdg_buffer;
    char*                _l1adg_buffer;
    SerializerDictionary _dict;
    QLabel*      _runInfo;
    QListWidget* _devlist;
    QListWidget* _cmplist;
    Pds::XtcFileIterator*          _fiter;
    std::vector<char*>             _cycle;
    int                            _icycle;
  };
};

#endif


        
        
