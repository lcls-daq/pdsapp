#include "NodeSelect.hh"

#include "pds/collection/PingReply.hh"
#include "pdsdata/xtc/DetInfo.hh"

#include <QtCore/QString>
#include <QtGui/QButtonGroup>
#include <QtGui/QVBoxLayout>

using namespace Pds;

NodeGroup::NodeGroup(const QString& label, QWidget* parent) :
  QGroupBox(label, parent),
  _buttons(new QButtonGroup(parent)) 
{
  _buttons->setExclusive(false);
  setLayout(new QVBoxLayout(this)); 
  connect(this, SIGNAL(node_added(int)), 
	  this, SLOT(add_node(int)));
}

NodeGroup::~NodeGroup() 
{
  delete _buttons; 
}

void NodeGroup::addNode(const NodeSelect& node)
{
  int index = _nodes.size();
  _nodes << node;
  emit node_added(index);
}

void NodeGroup::add_node(int index)
{ 
  const NodeSelect& node = _nodes[index];
  QCheckBox* button = new QCheckBox(node.label(),this);
  button->setCheckState(Qt::Checked);  // default to include
  layout()->addWidget(button); 
  _buttons->addButton(button,index); 
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


NodeSelect::NodeSelect(const Node& node) :
  _node    (node)
{
  _label = QString(Level::name(node.level()));
  struct in_addr inaddr;
  inaddr.s_addr = ntohl(node.ip());
  _label += QString(" : %1").arg(inet_ntoa(inaddr));
  _label += QString(" : %1").arg(node.pid());
}

NodeSelect::NodeSelect(const Node& node, const PingReply& msg) :
  _node    (node)
{
  if (msg.nsources()) {
    const DetInfo& src = static_cast<const DetInfo&>(msg.source(0));
    _label  = QString("%1/%2").arg(DetInfo::name(src.detector())).arg(src.detId());
    _label += QString("/%1/%2").arg(DetInfo::name(src.device  ())).arg(src.devId());
  }
  else 
    _label  = "Segment";
  struct in_addr inaddr;
  inaddr.s_addr = ntohl(node.ip());
  _label += QString(" : %1").arg(inet_ntoa(inaddr));
  _label += QString(" : %1").arg(node.pid());
}

NodeSelect::NodeSelect(const Node& node, const char* desc) :
  _node    (node)
{
  _label = desc;
  struct in_addr inaddr;
  inaddr.s_addr = ntohl(node.ip());
  _label += QString(" : %1").arg(inet_ntoa(inaddr));
  _label += QString(" : %1").arg(node.pid());
}

NodeSelect::NodeSelect(const NodeSelect& s) :
  _node (s._node),
  _label(s._label)
{
}

NodeSelect::~NodeSelect() 
{
}
