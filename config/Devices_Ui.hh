#ifndef Pds_Devices_Ui_hh
#define Pds_Devices_Ui_hh

#include "pdsapp/config/SerializerDictionary.hh"

#include <QtGui/QGroupBox>

#include <string>
using std::string;

class QListWidget;
class QLineEdit;
class QPushButton;
class QComboBox;

namespace Pds_ConfigDb {

  class Device;
  class Experiment;
  class Serializer;
  class UTypeName;

  class Devices_Ui : public QGroupBox {
    Q_OBJECT
  public:
    Devices_Ui(QWidget*, Experiment&, bool edit);
  public slots:
    void update_device_list();
    void new_device();
    void edit_device();
    void update_config_list();
    void update_component_list();
    void new_config();
    void copy_config();
    void change_component();
    void view_component();
    void add_component(const QString& type);
    void db_update();
  private:
    bool validate_config_name(const string& name);
  private:
    Device* _device() const;
    Serializer& lookup(const UTypeName&);
  private:
    Experiment&  _expt;
    SerializerDictionary _dict;
    QListWidget* _devlist;
    QLineEdit*   _devnewedit;
    QPushButton* _devnewbutton;
    QPushButton* _deveditbutton;
    QListWidget* _cfglist;
    QLineEdit*   _cfgnewedit;
    QPushButton* _cfgnewbutton;
    QLineEdit*   _cfgcpyedit;
    QPushButton* _cfgcpybutton;
    QListWidget* _cmplist;
    QComboBox*   _cmpcfglist;
  };
};

#endif


        
        
