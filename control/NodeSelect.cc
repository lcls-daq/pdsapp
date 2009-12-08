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
  button->setCheckState(Qt::Checked);  // default to include
  button->setPalette( node.ready() ? *_ready : *_notready );
  layout()->addWidget(button); 
  _buttons->addButton(button,index); 
  QObject::connect(button, SIGNAL(clicked()), this, SIGNAL(list_changed()));
  
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
  QList<Node> nodes;
  QList<QAbstractButton*> buttons = _buttons->buttons();
  foreach(QAbstractButton* b, buttons) {
    if (b->isChecked()) {
      int id = _buttons->id(b);
      nodes << _nodes[id].node();
    }
  }
  return nodes;
}

QList<DetInfo> NodeGroup::detectors() 
{
  QList<DetInfo> dets;
  QList<QAbstractButton*> buttons = _buttons->buttons();
  foreach(QAbstractButton* b, buttons) {
    if (b->isChecked()) {
      int id = _buttons->id(b);
      dets << _nodes[id].det();
    }
  }
  return dets;
}

NodeGroup* NodeGroup::freeze()
{
  NodeGroup* g = new NodeGroup(title(),(QWidget*)0);

  QList<QAbstractButton*> buttons = _buttons->buttons();
  foreach(QAbstractButton* b, buttons) {
    if (b->isChecked())
      g->addNode(_nodes[_buttons->id(b)]);
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
    const DetInfo& src = static_cast<const DetInfo&>(msg.source(0));
    _label  = QString("%1/%2").arg(DetInfo::name(src.detector())).arg(src.detId());
    _label += QString("/%1/%2").arg(DetInfo::name(src.device  ())).arg(src.devId());
    _det    = src;
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

bool NodeSelect::operator==(const NodeSelect& n) const { return n._node == _node; }

