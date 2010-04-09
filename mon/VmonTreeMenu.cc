#include "VmonTreeMenu.hh"
#include "MonTree.hh"
#include "MonTabMenu.hh"

#include "pds/service/Task.hh"

#include "pds/vmon/VmonRecorder.hh"
#include "pds/mon/MonClient.hh"
#include "pds/utility/Transition.hh"

#include <QtGui/QVBoxLayout>
#include <QtGui/QHBoxLayout>
#include <QtGui/QPushButton>
#include <QtGui/QButtonGroup>
#include <QtGui/QGroupBox>
#include <QtGui/QRadioButton>
#include <QtGui/QFileDialog>
#include <QtGui/QLineEdit>
#include <QtGui/QLabel>

namespace Pds {
  class ClientButton : public QRadioButton {
  public:
    ClientButton(MonClient& c, QWidget* parent) : 
      QRadioButton(c.cds().desc().name(), parent),
      client(c) {}
    ~ClientButton() {}
  public:
    MonClient& client;
  };
};

using namespace Pds;

class VmonTreeEvent : public Routine {
public:
  VmonTreeEvent(VmonTreeMenu& trees, 
		MonTree& tree,
		MonConsumerClient::Type type,
		int result) :
    _trees(trees),
    _tree(tree),
    _type(type),
    _result(result)
  {}
  virtual ~VmonTreeEvent() {}

  virtual void routine()
  {
    _trees.event(_tree, _type, _result);
    delete this;
  }

protected:
  VmonTreeMenu& _trees;
  MonTree& _tree;
  MonConsumerClient::Type _type;
  int _result;
};


VmonTreeMenu::VmonTreeMenu(QWidget& p, 
			   Task& task, 
			   MonTabMenu& tabs,
			   unsigned char platform,
			   const char*   partition) :
  QGroupBox (&p),
  VmonClientManager(platform, partition, *this),
  _task     (task),
  _tabs     (tabs),
  _selected (0),
  _partition_id(0),
  _last_transition(TransitionId::Unknown),
  _recorder (new VmonRecorder)
{
  QVBoxLayout* layout = new QVBoxLayout(this);

  char buff[32];
  QGroupBox* control = new QGroupBox("Control", this);
  { QVBoxLayout* clayout = new QVBoxLayout(control);
    sprintf(buff,"Partition: %s",partition);
    clayout->addWidget(new QLabel(buff, control));
    { QHBoxLayout* hlayout = new QHBoxLayout;
      hlayout->addWidget(new QLabel("Id:", control));
      _partition_id_edit = new QLineEdit("",control);
      hlayout->addWidget(_partition_id_edit);
      clayout->addLayout(hlayout);
    }
    sprintf(buff,"LastTr: %s",TransitionId::name(_last_transition));
    _transition_label   = new QLabel(buff,control);
    clayout->addWidget(_transition_label  );
    { QHBoxLayout* hlayout = new QHBoxLayout;
      QPushButton* startButton = new QPushButton("Start", control);
      QPushButton* stopButton  = new QPushButton("Stop", control);
      hlayout->addWidget(startButton);
      hlayout->addWidget(stopButton);
      clayout->addLayout(hlayout); 
      QObject::connect(startButton, SIGNAL(clicked()), this, SLOT(control_start()));
      QObject::connect(stopButton , SIGNAL(clicked()), this, SLOT(control_stop()));
    }
    control->setLayout(clayout); 
  }
  layout->addWidget(control);

  QGroupBox* record = new QGroupBox("Record", this);
  { QVBoxLayout* rlayout = new QVBoxLayout;
    _filename_label = new QLabel("", record);
    _filesize_label = new QLabel("", record);
    rlayout->addWidget(_filename_label);
    rlayout->addWidget(_filesize_label);
    { QHBoxLayout* hlayout = new QHBoxLayout;
      QPushButton* startButton = new QPushButton("Start", record);
      QPushButton* stopButton  = new QPushButton("Stop", record);
      hlayout->addWidget(startButton);
      hlayout->addWidget(stopButton);
      rlayout->addLayout(hlayout); 
      QObject::connect(startButton, SIGNAL(clicked()), this, SLOT(record_start()));
      QObject::connect(stopButton , SIGNAL(clicked()), this, SLOT(record_stop()));
    }
    record->setLayout(rlayout);
  }
  layout->addWidget(record);

  _client_bg     = new QButtonGroup(this);
  _client_bg_box = new QGroupBox("Display", this);
  _client_bg_box->setLayout(new QVBoxLayout(_client_bg_box));
  layout->addWidget(_client_bg_box);

  QObject::connect(_client_bg, SIGNAL(buttonClicked(QAbstractButton*)), this, SLOT(set_tree(QAbstractButton*)));

  QObject::connect(this, SIGNAL(control_updated()), this, SLOT(control_update()));
  QObject::connect(this, SIGNAL(record_updated()), this, SLOT(record_update()));
  QObject::connect(this, SIGNAL(client_added(void*)), 
		   this, SLOT(add_client(void*)));

  setLayout(layout);

  VmonClientManager::start();
  if (!CollectionManager::connect()) {
    printf("platform %x unavailable\n",platform);
    exit(-1);
  }
}

VmonTreeMenu::~VmonTreeMenu() 
{
}

void VmonTreeMenu::expired() 
{
  _recorder->flush();
  request_payload();
  emit record_updated();
}

void VmonTreeMenu::set_tree(QAbstractButton* b)
{
  ClientButton* c = static_cast<ClientButton*>(b);
  _tabs.reset(c->client);
}

void VmonTreeMenu::control_start()
{
  VmonClientManager::connect(_partition_id_edit->text().toInt());
  VmonClientManager::map();
}

void VmonTreeMenu::control_stop()
{
  VmonClientManager::disconnect();
}

void VmonTreeMenu::control_update()
{
  _partition_id_edit->setText(QString::number(_partition_id));

  QString last_transition("LastTr: ");
  last_transition += TransitionId::name(_last_transition);
  _transition_label->setText(last_transition);
}


void VmonTreeMenu::record_start()
{
  _recorder->enable();
}

void VmonTreeMenu::record_stop()
{
  _recorder->disable();
}

void VmonTreeMenu::record_update()
{
  _filename_label->setText(QString(_recorder->filename()));
  _filesize_label->setText(QString::number(_recorder->filesize())+" bytes");
}


void VmonTreeMenu::read_config()
{
  QString fname = 
    QFileDialog::getOpenFileName(this,
				 "Read Configuration From File",
				 "", "*.cnf");
  if (!fname.isNull()) {
    //    _tabs.readconfig(client(),qPrintable(fname));
  }
}

void VmonTreeMenu::write_config()
{
  QString fname = 
    QFileDialog::getSaveFileName(this,
				 "Write Configuration To File",
				 "", "*.cnf");
  if (!fname.isNull())
    _tabs.writeconfig(qPrintable(fname));
}

void VmonTreeMenu::event(MonTree& tree,
			 MonConsumerClient::Type type, 
			 int result)
{
  tree.event(type, result);
}

void VmonTreeMenu::process(MonClient& client, 
			   MonConsumerClient::Type type, 
			   int result)
{
  std::map<MonClient*,MonTree*>::iterator it = _map.find(&client);
  if (it != _map.end())
    _task.call(new VmonTreeEvent(*this,*(it->second), 
				 type, result));
  if (type==MonConsumerClient::Description)  _recorder->description(client);
  if (type==MonConsumerClient::Payload    )  _recorder->payload    (client);
}

void VmonTreeMenu::allocated(const Allocation& alloc,
			     unsigned index)
{
  _partition_id = alloc.partitionid();
}

void VmonTreeMenu::post(const Transition& tr)
{
  _last_transition = tr.id();
  VmonClientManager::post(tr);
  emit control_updated();
}

void VmonTreeMenu::add(MonClient& client)
{
  emit client_added(&client);
}

void VmonTreeMenu::add_client(void* cptr)
{
  MonClient& client = *reinterpret_cast<MonClient*>(cptr);
  ClientButton* button = new ClientButton(client, _client_bg_box);
  button->setChecked( false );
  _client_bg_box->layout()->addWidget(button);
  _client_bg->addButton(button);

  MonTree* tree = new MonTree(_tabs, client, 0);
  _trees.push_back(tree);
  _map.insert(std::pair<MonClient*,MonTree*>(&client,tree));
}

void VmonTreeMenu::clear()
{
  _map.clear();

  QList<QAbstractButton*> buttons = _client_bg->buttons();
  for(QList<QAbstractButton*>::iterator iter = buttons.begin();
      iter != buttons.end(); iter++) {
    _client_bg->removeButton(*iter);
    _client_bg_box->layout()->removeWidget(*iter);
    delete *iter;
  }
  
  VmonClientManager::clear();

  for(list<MonTree*>::iterator iter = _trees.begin();
      iter != _trees.end(); iter++) 
    delete (*iter);
  _trees.clear();

  update();
}
