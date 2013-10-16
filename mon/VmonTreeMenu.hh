#ifndef Pds_VmonTREEMENU_HH
#define Pds_VmonTREEMENU_HH

#include <QtGui/QGroupBox>
#include "pds/mon/MonConsumerClient.hh"
#include "pds/management/VmonClientManager.hh"

#include <list>
#include <map>
using std::list;

class QAbstractButton;
class QLineEdit;
class QButtonGroup;
class QGroupBox;
class QPushButton;
class QLabel;

namespace Pds {

  class Task;
  class MonClient;
  class MonTabMenu;
  class MonTree;
  class VmonRecorder;

  class VmonTreeMenu : public QGroupBox,
		       public MonConsumerClient,
		       public VmonClientManager {
    Q_OBJECT
  public:
    VmonTreeMenu(QWidget& p, 
		 Task& task,
		 MonTabMenu& tabs,
		 unsigned char platform,
		 const char*   partition,
		 const char*   path);
    virtual ~VmonTreeMenu();

    void expired();
    void event(MonTree&, MonConsumerClient::Type type, int result);

  signals:
    void control_updated();
    void record_updated();
    void client_added(void*);
    void cleared();
  private slots:
    void control_start();
    void control_stop();
    void control_update();

    void record_start ();
    void record_stop  ();
    void record_update();

    void read_config();
    void write_config();
    //    void change_config_file();
    void set_tree(QAbstractButton*);
    void add_client(void*);
    void clear_tabs();
  private:
    // Implements MonConsumerClient
    virtual void process(MonClient& client, MonConsumerClient::Type type, int result=0);
  public:
    // Implements VmonClient
    virtual void allocated(const Allocation&,
			   unsigned index);
    virtual void dissolved() {}
    virtual void post(const Transition&);
  private:
    void add(MonClient&);
    void clear();
  private:
    Task&        _task;
    MonTabMenu&  _tabs;
    list<MonTree*> _trees; 
    std::map<MonClient*,MonTree*> _map;
    int          _selected;
    QLineEdit*   _partition_id_edit;
    QLabel*      _transition_label;
    unsigned     _partition_id;
    TransitionId::Value _last_transition;
    QGroupBox*    _client_bg_box;
    QButtonGroup* _client_bg;
    VmonRecorder* _recorder;
    QLabel*       _filename_label;
    QLabel*       _filesize_label;
  };
};

#endif
