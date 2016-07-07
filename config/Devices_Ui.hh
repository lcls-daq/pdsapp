#ifndef Pds_Devices_Ui_hh
#define Pds_Devices_Ui_hh

#include "pdsapp/config/SerializerDictionary.hh"
#include "pdsapp/config/ExpertDictionary.hh"

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
    void remove_device();
    void edit_device();
    void view_device();
    void update_config_list();
    void update_component_list();
    void new_config();
    void copy_config();
    void remove_config();
    void change_component();
    void view_component();
    void add_component(const QString& type);
    void remove_component(const QString& type);
    void db_update();
    void expert_mode();
    void user_mode();
  signals:
    void db_changed();
  private:
    bool validate_config_name(const string& name);
    void _current_component(string& utype, string& uname);
  private:
    const Device* _device_c() const;
    Device* _device() const;
    Serializer * lookup(const UTypeName&, bool);
    void lookup_warning(void);
  private:
    Experiment&  _expt;
    SerializerDictionary _dict;
    ExpertDictionary     _xdict;
    QListWidget* _devlist;
    QLineEdit*   _devnewedit;
    QPushButton* _devnewbutton;
    QPushButton* _deveditbutton;
    QPushButton* _devrembutton;
    QListWidget* _cfglist;
    QLineEdit*   _cfgnewedit;
    QPushButton* _cfgnewbutton;
    QLineEdit*   _cfgcpyedit;
    QPushButton* _cfgcpybutton;
    QPushButton* _cfgrembutton;
    QListWidget* _cmplist;
    QComboBox*   _cmpaddlist;
    QComboBox*   _cmpremlist;
    bool         _expert_mode;
    bool         _edit;
  };
};

#endif


        
        
