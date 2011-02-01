#include "NodeSelect.hh"

#include "pds/collection/PingReply.hh"
#include "pdsdata/xtc/DetInfo.hh"

#include <QtCore/QString>
#include <QtGui/QButtonGroup>
#include <QtGui/QVBoxLayout>
#include <QtGui/QPalette>

using namespace Pds;

NodeGroup::NodeGroup(const QString& label, QWidget* parent) :
  QGroupBox(label, parent),
  _buttons(new QButtonGroup(parent)),
  _ready    (new QPalette(Qt::green)),
  _notready(new QPalette(Qt::red))
{
  //  Read persistent selected nodes
  char* buff = new char[256];
  sprintf(buff,".%s",qPrintable(title()));
  FILE* f = fopen(buff,"r");
  if (f) {
    printf("Opened %s\n",buff);
    char* lptr=buff;
    unsigned linesz;
    while(getline(&lptr,&linesz,f)!=-1) {
      QString p(lptr);
      p.chop(1);  // remove new-line
      _persist.push_back(p.replace('\t','\n'));
      printf("Persist %s\n",qPrintable(p));
    }
    fclose(f);
  }
  else {
    printf("Failed to open %s\n",buff);
  }
  delete[] buff;

  _buttons->setExclusive(false);
  setLayout(new QVBoxLayout(this)); 
  connect(this, SIGNAL(node_added(int)), 
	  this, SLOT(add_node(int)));
  connect(this, SIGNAL(node_replaced(int)), 
	  this, SLOT(replace_node(int)));
}

NodeGroup::~NodeGroup() 
{
  delete _buttons; 
}

void NodeGroup::addNode(const NodeSelect& node)
{
  int index = _nodes.indexOf(node);
  if (index >= 0) {
    _nodes.replace(index, node);
    emit node_replaced(index);
  }
  else {
    index = _nodes.size();
    _nodes << node;
    emit node_added(index);
  }
}

void NodeGroup::add_node(int index)
{ 
  const NodeSelect& node = _nodes[index];
  QCheckBox* button = new QCheckBox(node.label(),this);
  printf("Add node %s\n",qPrintable(node.plabel()));
  button->setCheckState(_persist.contains(node.plabel()) ? Qt::Checked : Qt::Unchecked);
  button->setPalette( node.ready() ? *_ready : *_notready );
  _buttons->addButton(button,index); 
  QObject::connect(button, SIGNAL(clicked()), this, SIGNAL(list_changed()));

  QBoxLayout* l = static_cast<QBoxLayout*>(layout());
  for(index = 0; index < l->count(); index++)
    if (node.label() < static_cast<QCheckBox*>(l->itemAt(index)->widget())->text())
      break;
  l->insertWidget(index,button); 
  
  emit list_changed();
}

void NodeGroup::replace_node(int index)
{ 
  const NodeSelect& node = _nodes[index];
  QCheckBox* button = static_cast<QCheckBox*>(_buttons->button(index));
  button->setText(node.label());
  button->setPalette( node.ready() ? *_ready : *_notready );

  emit list_changed();
}

QList<Node> NodeGroup::selected() 
{
  _persist.clear();
  QList<Node> nodes;
  QList<QAbstractButton*> buttons = _buttons->buttons();
  foreach(QAbstractButton* b, buttons) {
    if (b->isChecked()) {
      int id = _buttons->id(b);
      _persist.push_back(_nodes[id].plabel().replace('\n','\t'));
      if (_nodes[id].det().device()==DetInfo::Evr)
	nodes.push_front(_nodes[id].node());
      else
	nodes.push_back (_nodes[id].node());
    }
  }

  //  Write persistent selected nodes
  char* buff = new char[64];
  sprintf(buff,".%s",qPrintable(title()));
  FILE* f = fopen(buff,"w");
  if (f) {
    foreach(QString p, _persist) {
      fprintf(f,"%s\n",qPrintable(p));
    }
    fclose(f);
  }
  else {
    printf("Failed to open %s\n",buff);
  }
  delete[] buff;

  return nodes;
}

QList<DetInfo> NodeGroup::detectors() 
{
  QList<DetInfo> dets;
  QList<QAbstractButton*> buttons = _buttons->buttons();
  foreach(QAbstractButton* b, buttons) {
    if (b->isChecked()) {
      int id = _buttons->id(b);
      if (_nodes[id].det().device()==DetInfo::Evr)
	dets.push_front(_nodes[id].det());
      else
	dets.push_back (_nodes[id].det());
    }
  }
  return dets;
}

NodeGroup* NodeGroup::freeze()
{
  NodeGroup* g = new NodeGroup(title(),(QWidget*)0);

  {
    QList<QAbstractButton*> buttons = _buttons->buttons();
    foreach(QAbstractButton* b, buttons) {
      if (b->isChecked())
	g->addNode(_nodes[_buttons->id(b)]);
    }
  }

  {
    QList<QAbstractButton*> buttons = g->_buttons->buttons();
    foreach(QAbstractButton* b, buttons) {
      b->setEnabled(false);
    }
  }

  return g;
}

bool NodeGroup::ready() const
{
  QList<QAbstractButton*> buttons = _buttons->buttons();
  for(int i=0; i<buttons.size(); i++)
    if (buttons[i]->isChecked() && !_nodes[i].ready())
      return false;
  return true;
}

NodeSelect::NodeSelect(const Node& node) :
  _node    (node),
  _ready   (true)
{
  _label = QString(Level::name(node.level()));
  struct in_addr inaddr;
  inaddr.s_addr = ntohl(node.ip());
  _label += QString(" : %1").arg(inet_ntoa(inaddr));
  _label += QString(" : %1").arg(node.pid());
}

NodeSelect::NodeSelect(const Node& node, const PingReply& msg) :
  _node    (node),
  _ready   (msg.ready())
{
  if (msg.nsources()) {
    {
      const DetInfo& src = static_cast<const DetInfo&>(msg.source(0));
      _label  = QString("%1/%2").arg(DetInfo::name(src.detector())).arg(src.detId());
      _label += QString("/%1/%2").arg(DetInfo::name(src.device  ())).arg(src.devId());
      _det    = src;
    }
    for(unsigned i=1; i<msg.nsources(); i++) {
      const DetInfo& src = static_cast<const DetInfo&>(msg.source(i));
      _label += QString("\n%1/%2").arg(DetInfo::name(src.detector())).arg(src.detId());
      _label += QString("/%1/%2").arg(DetInfo::name(src.device  ())).arg(src.devId());
    }
  }
  else 
    _label = QString(Level::name(node.level()));
  struct in_addr inaddr;
  inaddr.s_addr = ntohl(node.ip());
  _label += QString(" : %1").arg(inet_ntoa(inaddr));
  _label += QString(" : %1").arg(node.pid());
}

NodeSelect::NodeSelect(const Node& node, const char* desc) :
  _node    (node),
  _ready   (true)
{
  _label = desc;
  struct in_addr inaddr;
  inaddr.s_addr = ntohl(node.ip());
  _label += QString(" : %1").arg(inet_ntoa(inaddr));
  _label += QString(" : %1").arg(node.pid());
}

NodeSelect::NodeSelect(const NodeSelect& s) :
  _node (s._node),
  _det  (s._det ),
  _label(s._label),
  _ready(s._ready)
{
}

NodeSelect::~NodeSelect() 
{
}

bool NodeSelect::operator==(const NodeSelect& n) const 
{
  return n._node == _node; 
}

QString NodeSelect::plabel() const
{
  return _label.mid(0,_label.lastIndexOf(':'));
}
