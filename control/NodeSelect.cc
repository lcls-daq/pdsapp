#include "NodeSelect.hh"

#include "pds/collection/PingReply.hh"
#include "pdsdata/xtc/BldInfo.hh"
#include "pdsdata/xtc/DetInfo.hh"

#include <QtCore/QString>
#include <QtGui/QButtonGroup>
#include <QtGui/QVBoxLayout>
#include <QtGui/QPalette>
#include <errno.h>

#define NODE_BUFF_SIZE  256

using namespace Pds;

static QList<int> _bldOrder = 
  QList<int>() << BldInfo::EBeam
	       << BldInfo::PhaseCavity
	       << BldInfo::FEEGasDetEnergy
	       << BldInfo::Nh2Sb1Ipm01
	       << BldInfo::HxxUm6Imb01
	       << BldInfo::HxxUm6Imb02                           
	       << BldInfo::HxxDg1Cam
	       << BldInfo::HfxDg2Imb01
	       << BldInfo::HfxDg2Imb02
	       << BldInfo::HfxDg2Cam
	       << BldInfo::HfxMonImb01
	       << BldInfo::HfxMonImb02
	       << BldInfo::HfxMonCam
	       << BldInfo::HfxDg3Imb01
	       << BldInfo::HfxDg3Imb02
	       << BldInfo::HfxDg3Cam
	       << BldInfo::XcsDg3Imb03
	       << BldInfo::XcsDg3Imb04
	       << BldInfo::XcsDg3Cam
	       << BldInfo::XcsDg3Cam
	       << BldInfo::NumberOf;


NodeGroup::NodeGroup(const QString& label, QWidget* parent, unsigned platform) :
  QGroupBox(label, parent),
  _buttons(new QButtonGroup(parent)),
  _ready    (new QPalette(Qt::green)),
  _notready(new QPalette(Qt::red)),
  _platform(platform)
{
  //  Read persistent selected nodes
  char *buff = (char *)malloc(NODE_BUFF_SIZE);  // use malloc w/ getline
  if (buff == (char *)NULL) {
    printf("%s: malloc(%d) failed, errno=%d\n", __PRETTY_FUNCTION__, NODE_BUFF_SIZE, errno);
  } else {
    snprintf(buff, NODE_BUFF_SIZE-1, ".%s for platform %u", qPrintable(title()), _platform);
    FILE* f = fopen(buff,"r");
    if (f) {
      printf("Opened %s\n",buff);
      char* lptr=buff;
      unsigned linesz = NODE_BUFF_SIZE;         // initialize for getline
      while(getline(&lptr,&linesz,f)!=-1) {
        QString p(lptr);
        p.chop(1);  // remove new-line
        _persist.push_back(p.replace('\t','\n'));
        printf("Persist %s\n",qPrintable(p));
      }
      fclose(f);
    }
    else {
      printf("Failed to open %s\n", buff);
    }
    free(buff);

    _buttons->setExclusive(false);
    setLayout(new QVBoxLayout(this)); 
    connect(this, SIGNAL(node_added(int)), 
            this, SLOT(add_node(int)));
    connect(this, SIGNAL(node_replaced(int)), 
            this, SLOT(replace_node(int)));
  }
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
  if (node.src().level()==Level::Reporter) {
    int order = _bldOrder.indexOf(node.src().phy());
    for(index = 0; index < l->count(); index++) {
      if (order < _order[index])
        break;
    }
    _order.insert(index,order);
  }
  else {
    for(index = 0; index < l->count(); index++) {
      if (node.label() < static_cast<QCheckBox*>(l->itemAt(index)->widget())->text())
        break;
    }
  }
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
  char buff[64];
  snprintf(buff, sizeof(buff)-1, ".%s for platform %u", qPrintable(title()), _platform);
  FILE* f = fopen(buff,"w");
  if (f) {
    foreach(QString p, _persist) {
      fprintf(f,"%s\n",qPrintable(p));
    }
    fclose(f);
  }
  else {
    printf("Failed to open %s\n", buff);
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
      if (_nodes[id].det().device()==DetInfo::Evr)
        dets.push_front(_nodes[id].det());
      else
        dets.push_back (_nodes[id].det());
    }
  }
  return dets;
}

QList<BldInfo> NodeGroup::reporters() 
{
  QList<BldInfo> dets;
  QList<QAbstractButton*> buttons = _buttons->buttons();
  foreach(QAbstractButton* b, buttons) {
    if (b->isChecked()) {
      int id = _buttons->id(b);
      dets.push_back(_nodes[id].bld());
    }
  }
  return dets;
}

NodeGroup* NodeGroup::freeze()
{
  NodeGroup* g = new NodeGroup(title(),(QWidget*)0, 0);

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
    bool found=false;
    for(unsigned i=0; i<msg.nsources(); i++) {
      if (msg.source(i).level()==Level::Source) {
        const DetInfo& src = static_cast<const DetInfo&>(msg.source(i));
        if (!found) {
          found=true;
          _src    = msg.source(i);
          _label  = QString("%1/%2").arg(DetInfo::name(src.detector())).arg(src.detId());
        }
        else
          _label += QString("\n%1/%2").arg(DetInfo::name(src.detector())).arg(src.detId());
        _label += QString("/%1/%2").arg(DetInfo::name(src.device  ())).arg(src.devId());
      }
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

NodeSelect::NodeSelect(const Node& node, const BldInfo& info) :
  _node  (node),
  _src   (info),
  _label (BldInfo::name(info)),
  _ready (true)
{
}

NodeSelect::NodeSelect(const NodeSelect& s) :
  _node (s._node),
  _src  (s._src ),
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
