#include "MonTreeMenu.hh"
#include "MonTree.hh"
#include "MonTabMenu.hh"

#include "pds/service/Task.hh"

#include "pds/mon/MonClientManager.hh"
#include "pds/mon/MonClient.hh"

#include <QtGui/QVBoxLayout>
#include <QtGui/QHBoxLayout>
#include <QtGui/QPushButton>
#include <QtGui/QButtonGroup>
#include <QtGui/QRadioButton>
#include <QtGui/QFileDialog>

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
  _clientmanager(new MonClientManager(*this, hosts)),
  _selected(0)
{
  _trees = new MonTree*[_clientmanager->nclients()];
  for(unsigned c = 0; c<_clientmanager->nclients(); c++) {
    _trees[c] = new MonTree(_tabs,
			    *_clientmanager->client(c),
			    _clientmanager);
  }

  QVBoxLayout* layout = new QVBoxLayout(this);

  QPushButton* start = new QPushButton("Start", this);
  connect(start, SIGNAL(clicked()), this, SLOT(start_stop()));
  layout->addWidget(start);

  QGroupBox* cfg = new QGroupBox("Configuration", this);
  QHBoxLayout* rw_layout = new QHBoxLayout(cfg);
  QPushButton* readb  = new QPushButton("Read" , cfg);
  QPushButton* writeb = new QPushButton("Write", cfg);
  rw_layout->addWidget(readb);
  rw_layout->addWidget(writeb);
  connect(readb , SIGNAL(clicked()), this, SLOT(read_config()));
  connect(writeb, SIGNAL(clicked()), this, SLOT(write_config()));
  cfg->setLayout(rw_layout);
  layout->addWidget(cfg);

  unsigned nclients = _clientmanager->nclients();
  QButtonGroup* client_bg = new QButtonGroup(this);
  for (unsigned c=0; c<nclients; c++) {
    MonClient* client = _clientmanager->client(c);
    QRadioButton* button = 
      new QRadioButton(client->cds().desc().name(),this);
    client_bg->addButton(button,c);
    button->setChecked( c==(unsigned)_selected ? true : false);
    layout->addWidget(button);
  }
  //  connect(client_bg, SIGNAL(buttonClicked(int)), this, SLOT(set_tree(int)));

  layout->addStretch();

  setLayout(layout);

  _start_stop = start;
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
  _tabs.reset(*_clientmanager->client(_selected=c));
}

void MonTreeMenu::event(MonClient& client, Type type, int result)
{
  _trees[client.src().phy()]->event(type, result);
}

void MonTreeMenu::start_stop()
{
  if (_trees[_selected]->is_connected()) {
    _start_stop->setText("Start");
    _trees[_selected]->disconnect();
  }
  else {
    _start_stop->setText("Stop");
    _trees[_selected]->connect();
  }
}

void MonTreeMenu::read_config()
{
  QString fname = 
    QFileDialog::getOpenFileName(this,
				 "Read Configuration From File",
				 "", "*.cnf");
  if (!fname.isNull()) {
    for (unsigned c=0; c<_clientmanager->nclients(); c++) {
      MonClient* client = _clientmanager->client(c);
      _tabs.readconfig(*client,qPrintable(fname));
    }
  }
}

void MonTreeMenu::write_config()
{
  QString fname = 
    QFileDialog::getSaveFileName(this,
				 "Write Configuration To File",
				 "", "*.cnf");
  if (!fname.isNull())
    _tabs.writeconfig(qPrintable(fname));
}

void MonTreeMenu::process(MonClient& client, Type type, int result)
{
  _task.call(new MonTreeEvent(*this, client, type, result));
}

