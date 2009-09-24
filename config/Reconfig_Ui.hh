#ifndef Pds_Reconfig_Ui_hh
#define Pds_Reconfig_Ui_hh

#include "pdsapp/config/SerializerDictionary.hh"

#include <QtGui/QDialog>

#include <string>
using std::string;

class QListWidget;

namespace Pds_ConfigDb {

  class Device;
  class Experiment;
  class Serializer;
  class TableEntry;
  class UTypeName;

  class Reconfig_Ui : public QDialog {
    Q_OBJECT
  public:
    Reconfig_Ui(QWidget*, Experiment&);
  public slots:
    void apply();
    void set_run_type(const QString&);
    void update_device_list();
    void update_component_list();
    void change_component();
  private:
    Device* _device() const;
    const TableEntry* _device_entry() const;
    Serializer& lookup(const UTypeName&);
  signals:
    void changed();
  private:
    Experiment&  _expt;
    const TableEntry* _entry;
    SerializerDictionary _dict;
    QListWidget* _devlist;
    QListWidget* _cmplist;
  };
};

#endif


        
        
