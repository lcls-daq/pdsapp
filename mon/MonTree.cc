#include <errno.h>

#include "MonTree.hh"
#include "MonPath.hh"
//#include "MonLayoutHints.hh"
#include "MonTabs.hh"

#include "pds/mon/MonClientManager.hh"
#include "pds/mon/MonClient.hh"
#include "pds/mon/MonCds.hh"
#include "pds/mon/MonEntry.hh"
#include "pds/mon/MonGroup.hh"
#include "pds/mon/MonDescEntry.hh"

static const unsigned UpdateRate=1;
static const unsigned RetryRate=10;

using namespace Pds;

MonTree::MonTree(MonTabs& tabs, 
		 MonClient&  client,
		 MonClientManager* clientmanager) :
  _status   (Disconnected),
  _update   (0),
  _retry    (0),
  _needretry(false),
  _tabs     (tabs),
  _client   (client),
  _clientmanager(clientmanager)
{
  QObject::connect(this, SIGNAL(ready()), this, SLOT(reset()));
}

MonTree::~MonTree() {}

bool MonTree::is_connected() const 
{
  return _status != Disconnected;
}

void MonTree::connect()
{
  _needretry = true;
  _clientmanager->connect(_client);
}

void MonTree::disconnect()
{
  _needretry = false;
  _clientmanager->disconnect(_client);
}

void MonTree::expired() 
{
  if (++_update == UpdateRate) {
    if (_status == Ready) {
      if (_client.needspayload()) {
	if (_client.askload() < 0) {
	  printf("*** MonTree::expired client [%d] askload error: %s\n", 
		 _client.socket().socket(), strerror(errno));
	} else {
	  _status = Waiting;
	}
      }
    } else if (_status == Disconnected) {
      if (_needretry && ++_retry == RetryRate && _clientmanager) {
	connect();
	_retry = 0;
      }
    }
    _update = 0;
  }
}

void MonTree::event(MonConsumerClient::Type type, int result)
{
  switch (type) {
  case MonConsumerClient::ConnectError:
    title("disconnected");
    _status = Disconnected;
    printf("*** MonTree::event %s connect error: %s\n", 
	   _client.cds().desc().name(), strerror(result));
    break;
  case MonConsumerClient::DisconnectError:
    title("disconnected");
    _status = Disconnected;
    printf("*** MonTree::event %s disconnect error: %s\n", 
	   _client.cds().desc().name(), strerror(result));
    break;
  case MonConsumerClient::Connected:
    title("connected");
    _status = Connected;
    _client.askdesc();
    break;
  case MonConsumerClient::Disconnected:
    title("disconnected");
    _status = Disconnected;
    break;
  case MonConsumerClient::Disabled:
    title("disabled");
    _status = Disabled;
    break;
  case MonConsumerClient::Enabled:
    title("enabled");
    _status = Enabled;
    _client.askdesc();
    break;
  case MonConsumerClient::Description:
    title("ready");
    _status = Ready;
    emit ready();
    break;
  case MonConsumerClient::Payload:
    _status = Ready;
    _tabs.update(true);
    break;
  default:
    printf("*** MonTree::event unable to handle event %d\n", type);
  }
}

void MonTree::title(const char* name)
{
  static const unsigned MaxLen=64;
  char tmp[MaxLen];
  snprintf(tmp, MaxLen, "%s: %s: %d", _client.cds().desc().name(), name, _status);
  //  setTitle(tmp);
  //  printf("%s\n",tmp);
}

void MonTree::reset()
{
  //  _tabs.clear();
  //  _tabs.setup(_client.cds(),0);
}
