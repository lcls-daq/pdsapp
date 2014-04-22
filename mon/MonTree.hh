#ifndef Pds_MonTREE_HH
#define Pds_MonTREE_HH

#include <QtCore/QObject>

#include "pds/mon/MonConsumerClient.hh"
#include "pdsdata/xtc/Level.hh"

namespace Pds {

  class MonTabs;
  class MonClient;
  class MonClientManager;
  
  class MonTree : public QObject {
    Q_OBJECT
  public:
    MonTree(MonTabs& tabs, 
	    MonClient& client,
	    MonClientManager* clientmanager=0);
    virtual ~MonTree();

    bool is_connected() const;

    void connect   ();
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
    Status      _status;
    unsigned    _update;
    unsigned    _retry;
    bool        _needretry;
    MonTabs& _tabs;
    MonClient&  _client;
    MonClientManager* _clientmanager;
  };
};

#endif
