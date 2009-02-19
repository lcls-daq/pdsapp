#ifndef Pds_MonTREE_HH
#define Pds_MonTREE_HH

#include <QtCore/QObject>

#include "pds/mon/MonConsumerClient.hh"

namespace Pds {

  class MonTabMenu;
  class MonClient;
  class MonClientManager;
  
  class MonTree : public QObject {
    Q_OBJECT
  public:
    MonTree(MonTabMenu& tabs, 
	    MonClientManager& clientmanager,
	    MonClient& client);
    virtual ~MonTree();

    void connect();
    void disconnect();
    void expired();
    void event(MonConsumerClient::Type type, int result);

  private:
    void title(const char* name);

  signals:
    void ready();

  private slots:
    void reset();

  private:
    enum Status {Disconnected, Connected, Enabled, Disabled, Ready, Waiting};
    Status _status;
    unsigned _update;
    unsigned _retry;
    bool _needretry;
    MonTabMenu& _tabs;
    MonClientManager& _clientmanager;
    MonClient& _client;
  };
};

#endif
