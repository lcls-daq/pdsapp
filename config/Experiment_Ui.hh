#ifndef Pds_ConfigDb_Experiment_Ui_hh
#define Pds_ConfigDb_Experiment_Ui_hh

#include <QtGui/QGroupBox>

class QLineEdit;
class QListWidget;
class QPushButton;
class QComboBox;

#include <string>
using std::string;

namespace Pds_ConfigDb {

  class Experiment;

  class Experiment_Ui : public QGroupBox {
    Q_OBJECT
  public:
    Experiment_Ui(QWidget* parent,
		  Experiment& expt,
		  bool edit);
    ~Experiment_Ui();
  public slots:
    void update_device_list();
    void new_config();
    void copy_config();
    void device_changed();
    void add_device(const QString& name);
    void db_update();
  private:
    void change_device(const string&);
    void update_config_list();
    bool validate_config_name(const string&);
  private:
    Experiment&  _expt;
    QListWidget* _cfglist;
    QLineEdit*   _cfgnewedit;
    QPushButton* _cfgnewbutton;
    QLineEdit*   _cfgcopyedit;
    QPushButton* _cfgcopybutton;
    QListWidget* _devlist;
    QComboBox*   _devcfglist;
  };
};

#endif
