#include "MonTreeMenu.hh"
#include "MonTree.hh"
#include "MonTabMenu.hh"

#include "pds/service/Task.hh"

#include "pds/mon/MonClientManager.hh"
#include "pds/mon/MonClient.hh"

#include <QtGui/QVBoxLayout>
#include <QtGui/QPushButton>
#include <QtGui/QTextEdit>
#include <QtGui/QLabel>
#include <QtGui/QButtonGroup>
#include <QtGui/QRadioButton>

using namespace Pds;

class MonTreeEvent : public Routine {
public:
  MonTreeEvent(MonTreeMenu& trees, 
	       MonClient& client, 
	       MonConsumerClient::Type type,
	       int result) :
    _trees(trees),
    _client(client),
    _type(type),
    _result(result)
  {}
  virtual ~MonTreeEvent() {}

  virtual void routine()
  {
    _trees.event(_client, _type, _result);
    delete this;
  }

protected:
  MonTreeMenu& _trees;
  MonClient& _client;
  MonConsumerClient::Type _type;
  int _result;
};


MonTreeMenu::MonTreeMenu(QWidget& p, 
			 Task& task, 
			 MonTabMenu& tabs,
			 const char** hosts,
			 const char* config) :
  QGroupBox(&p),
  _task(task),
  _tabs(tabs),
  _clientmanager(new MonClientManager(*this, hosts))
{
  _trees = new MonTree*[_clientmanager->nclients()];
  for(unsigned c = 0; c<_clientmanager->nclients(); c++) {
    _trees[c] = new MonTree(_tabs,
			    *_clientmanager,
			    *_clientmanager->client(c));
  }

  QVBoxLayout* layout = new QVBoxLayout(this);

  QPushButton* start = new QPushButton("Start", this);
  connect(start, SIGNAL(clicked()), this, SLOT(start()));
  layout->addWidget(start);

  QGroupBox* cfg = new QGroupBox("Configuration", this);
  QWidget* rw_line = new QWidget(cfg);
  { QPushButton* readb  = new QPushButton("Read" , rw_line);
    QPushButton* writeb = new QPushButton("Write", rw_line);
    QHBoxLayout* rw_layout = new QHBoxLayout(rw_line);
    rw_layout->addWidget(readb);
    rw_layout->addWidget(writeb);
    rw_line->setLayout(rw_layout);
    connect(readb , SIGNAL(clicked()), this, SLOT(read_config()));
    connect(writeb, SIGNAL(clicked()), this, SLOT(write_config()));
  }

  QVBoxLayout* cfg_layout = new QVBoxLayout(cfg);
  cfg_layout->addWidget(rw_line);
  cfg_layout->addWidget(new QLabel("Config name: ",cfg));
  cfg_layout->addWidget(_fn_edit = new QTextEdit(config,cfg));
  cfg->setLayout(cfg_layout);
  layout->addWidget(cfg);

  unsigned nclients = _clientmanager->nclients();
  QButtonGroup* client_bg = new QButtonGroup(this);
  for (unsigned c=0; c<nclients; c++) {
    MonClient* client = _clientmanager->client(c);
    QRadioButton* button = 
      new QRadioButton(client->cds().desc().name(),this);
    client_bg->addButton(button,c);
    layout->addWidget(button);
  }
  connect(client_bg, SIGNAL(buttonClicked(int)), this, SLOT(set_tree(int)));

  setLayout(layout);
}

MonTreeMenu::~MonTreeMenu() {}

void MonTreeMenu::expired() 
{
  unsigned nclients = _clientmanager->nclients();
  for (unsigned c=0; c<nclients; c++) {
    _trees[c]->expired();
  }
}

void MonTreeMenu::set_tree(int c)
{
  _tabs.reset(*_clientmanager->client(c));
}

void MonTreeMenu::event(MonClient& client, Type type, int result)
{
  _trees[client.id()]->event(type, result);
}

void MonTreeMenu::start()
{
  //  _start.SetState(kButtonEngaged);
  //  _start.SetText("Stop");
  for (unsigned c=0; c<_clientmanager->nclients(); c++) {
    _trees[c]->connect();
  }
}

void MonTreeMenu::stop()
{
  //  _start.SetState(kButtonUp);
  //  _start.SetText("Start");
  for (unsigned c=0; c<_clientmanager->nclients(); c++) {
    _trees[c]->disconnect();
  }
}

void MonTreeMenu::read_config()
{
  const char* name = qPrintable(_fn_edit->toPlainText());
  for (unsigned c=0; c<_clientmanager->nclients(); c++) {
    MonClient* client = _clientmanager->client(c);
    _tabs.readconfig(*client,name);
  }
}

void MonTreeMenu::write_config()
{
  const char* name = qPrintable(_fn_edit->toPlainText());
  _tabs.writeconfig(name);
}

void MonTreeMenu::process(MonClient& client, Type type, int result)
{
  _task.call(new MonTreeEvent(*this, client, type, result));
}

