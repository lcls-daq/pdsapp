#ifndef Pds_VmonTREEMENU_HH
#define Pds_VmonTREEMENU_HH

#include <QtGui/QGroupBox>
#include "pds/mon/MonConsumerClient.hh"
#include "pds/management/VmonClientManager.hh"

#include <list>
#include <map>
using std::list;

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

  class VmonTreeMenu : public QGroupBox,
		       public MonConsumerClient,
		       public VmonClientManager {
    Q_OBJECT
  public:
    VmonTreeMenu(QWidget& p, 
		 Task& task,
		 MonTabMenu& tabs,
		 unsigned char platform,
		 const char*   partition);
    virtual ~VmonTreeMenu();

    void expired();
    void event(MonTree&, MonConsumerClient::Type type, int result);

  signals:
    void updated();
    void client_added(void*);
  public slots:
    void control_start();
    void control_stop();
    void update_control();

    void read_config();
    void write_config();
    //    void change_config_file();
    void set_tree(int);
    void add_client(void*);

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
  };
};

#endif
