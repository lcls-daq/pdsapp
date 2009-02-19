#ifndef Pds_MonTREEMENU_HH
#define Pds_MonTREEMENU_HH

#include <QtGui/QGroupBox>

#include "pds/mon/MonConsumerClient.hh"

class QTextEdit;

namespace Pds {

  class Task;
  class MonTabMenu;
  class MonTree;
  class MonClientManager;

  class MonTreeMenu : public QGroupBox,
		      public MonConsumerClient {
    Q_OBJECT
  public:
    MonTreeMenu(QWidget& p, 
		Task& task,
		MonTabMenu& tabs,
		const char** hosts,
		const char* config);
    virtual ~MonTreeMenu();

    void expired();
    void event(MonClient& client, MonConsumerClient::Type type, int result);

  public slots:
    void start();
    void stop();
    void read_config();
    void write_config();
    //    void change_config_file();
    void set_tree(int);

  private:
    // Implements MonConsumerClient
    virtual void process(MonClient& client, MonConsumerClient::Type type, int result=0);

  private:
    QTextEdit* _fn_edit;

  private:
    Task& _task;
    MonTabMenu& _tabs;
    MonClientManager* _clientmanager;
    MonTree** _trees; 
  };
};

#endif
