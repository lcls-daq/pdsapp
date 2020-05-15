#include "pdsapp/control/NodeGroup.hh"
#include "pdsapp/control/Preferences.hh"

#include "pds/config/EvrConfigType.hh"

#include "pdsdata/xtc/BldInfo.hh"
#include "pdsdata/xtc/DetInfo.hh"

#include <QtCore/QString>
#include <QtGui/QButtonGroup>
#include <QtGui/QVBoxLayout>
#include <QtGui/QPalette>
#include <QtGui/QGroupBox>
#include <QtGui/QCheckBox>
#include <QtGui/QComboBox>
#include <QtGui/QLabel>

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

using namespace Pds;

static const char* cTransient = "Monitor";
static const char* cRecord    = "Record";
static const char* cSelected    = "Selected";
static const char* cNotSelected = "NotSelected";

NodeGroup::NodeGroup(const QString& label, QWidget* parent, unsigned platform, bool useGroups, bool useTransient) :
  QWidget  (parent),
  _group   (new QGroupBox(label)),
  _buttons (new QButtonGroup(parent)),
  _ready   (new QPalette(Qt::green)),
  _notready(new QPalette(Qt::red)),
  _palette (new QPalette()),
  _platform(platform),
  _useGroups(useGroups),
  _useTransient    (useTransient)
{
  //  Read persistent selected nodes
  //printf("NodeGroup::NodeGroup useGroups %c\n",useGroups?'t':'f');

  _read_pref(title(), _persist, _persistGroup, _persistTrans, _persistSelect);
  //_read_pref(QString("%1 required").arg(title()), 
         //_persist, _persistGroup, _persistTrans, _persistSelect);

  _notfound = _persist;

  _buttons->setExclusive(false);

  _group->setLayout(new QVBoxLayout);
  QVBoxLayout* l = new QVBoxLayout;
  l->addWidget(_group);
  _notfoundlist = new QLabel;

  _palette->setColor(QPalette::WindowText,Qt::red);
  _notfoundlist->setPalette(*_palette);	

  _build_notfoundlist(QString());
 
  l->addWidget(_notfoundlist);

  l->addStretch();
  setLayout(l);

  connect(this, SIGNAL(node_added(int)), 
          this, SLOT(add_node(int)));
  connect(this, SIGNAL(node_replaced(int)), 
          this, SLOT(replace_node(int)));
}

NodeGroup::~NodeGroup() 
{
  delete _buttons; 
  delete _ready;
  delete _notready;
  delete _palette;
  { NodeTransientCb* pCallback;
    foreach (pCallback, _lTransientCb)
      delete pCallback; }
}

void NodeGroup::addNode(const NodeSelect& node)
{
  int index = _nodes.indexOf(node);
  if (index >= 0) {                  
    NodeSelect old(_nodes[index]); // Capture user preferences
    _nodes.replace(index, node);
    setGroup    (index, old.node().group());
    setTransient(index, old.node().transient());
    emit node_replaced(index);
  }
  else {
    index = _nodes.size();
    _nodes << node;
    emit node_added(index);
  }
}

void NodeGroup::addChildNode(const NodeSelect& node, const NodeSelect& child)
{
  int index = _nodes.indexOf(node);
  if (index >= 0) {
    NodeSelect old(_nodes[index]); // Capture user preferences
    _nodes.replace(index, node);
    addChild(index, child);
    setGroup    (index, old.node().group());
    setTransient(index, old.node().transient());
    addChild(index, child);
    emit node_replaced(index);
  }
  else {
    index = _nodes.size();
    _nodes << node;
    addChild(index, child);
    QList<NodeSelect>::iterator it;
    QList<NodeSelect>& children = _children[index];
    for(it = children.begin(); it != children.end(); ++it) {
      it->node().setGroup    (this->node(index).group());
      it->node().setTransient(this->node(index).transient());
    }
    emit node_added(index);
  }
}

unsigned NodeGroup::nodes() const
{
  return _nodes.size();
}

QString NodeGroup::title() const
{
  return _group->title();
}

void NodeGroup::add_node(int index)
{ 
  const int iNodeIndex = index;
  const NodeSelect& node = _nodes[index];
  QCheckBox* button;
  if (node.alias().size()) {
    button = new QCheckBox(node.alias(),this);
    button->setToolTip(node.label());
  } else {
    button = new QCheckBox(node.label(),this);
  }
    
  int indexPersist = _persist.indexOf(node.plabel());
  int indexRequire = _require.indexOf(node.plabel());

  if ((indexPersist >= 0 && _persistSelect[indexPersist] == true) || indexRequire>=0)  {
    button->setCheckState(Qt::Checked);
  }
  else 	{
    button->setCheckState(Qt::Unchecked);
  }

  _build_notfoundlist(node.plabel());	

  button->setEnabled   ( indexRequire<0 );
  button->setPalette( isReady(index) ? *_ready : *_notready );
  _buttons->addButton(button,index); 
  QObject::connect(button, SIGNAL(clicked()), this, SIGNAL(list_changed()));

  index = order(node,button->text());

  QHBoxLayout* layoutButton = new QHBoxLayout;
  layoutButton->addWidget(button); 

  bool bEvrNode = (node.src().level() == Level::Source &&
		   static_cast<const DetInfo&>(node.src()).device() == DetInfo::Evr);          

  {

    QComboBox* transientBox = new QComboBox(this);
    transientBox->addItem("Rec");
    transientBox->addItem("Mon");
    transientBox->setVisible(_useTransient);
    layoutButton->addStretch(1);
    layoutButton->addWidget(transientBox);

    _lTransientCb.append(new NodeTransientCb(*this, iNodeIndex, *transientBox, button->checkState()==Qt::Checked));

    if (bEvrNode) {
      transientBox->setCurrentIndex(0);
      _lTransientCb.last()->stateChanged(0);
      transientBox->setEnabled(false);
    }
    else {
      connect(button,       SIGNAL(toggled(bool)),            _lTransientCb.last(), SLOT(selectChanged(bool)));
      connect(transientBox, SIGNAL(currentIndexChanged(int)), _lTransientCb.last(), SLOT(stateChanged(int)));    

      bool lTr = (indexPersist>=0) ? _persistTrans[indexPersist] : false;
      int cIndex = lTr ? 1:0;
      transientBox->setCurrentIndex(cIndex);
      _lTransientCb.last()->stateChanged(cIndex);
      transientBox->setEnabled(true);
    }
    while (_transients.size()<=iNodeIndex)
      _transients.push_back(0);
    _transients[iNodeIndex] = transientBox;
  }
    
  {    
    QComboBox* ciUseReadoutGroup = new QComboBox(this);
            
    bool bEvrNode = ( 
      node.src().level() == Level::Source &&
      static_cast<const DetInfo&>(node.src()).device() == DetInfo::Evr);               
    bool bBldNode = ( 
      node.src().level() == Level::Source &&
      static_cast<const DetInfo&>(node.src()).detector() == DetInfo::BldEb);      
    int iNodeGroup;
    if ( bEvrNode || bBldNode )
    {
      iNodeGroup = 0;
      ciUseReadoutGroup->addItem(QString().setNum(0));
      ciUseReadoutGroup->setCurrentIndex(0);    
      ciUseReadoutGroup->setDisabled(true);
    }
    else
    {
      iNodeGroup = ( indexPersist >= 0 ? _persistGroup[indexPersist] : 1 );            
      if (iNodeGroup == 0) {
        printf("NodeGroup::add_node _persistGroup=0 for non EVR/BLD node [%s].  Defaulting to group 1.\n",
               qPrintable(_persist[indexPersist]));
        iNodeGroup = 1;
      }
      for (int iGroup=1; iGroup <= EventCodeType::MaxReadoutGroup; ++iGroup)
        ciUseReadoutGroup->addItem(QString().setNum(iGroup));
      ciUseReadoutGroup->setCurrentIndex( iNodeGroup-1 );    
    }
        
    layoutButton->addStretch();         
    layoutButton->addWidget(ciUseReadoutGroup); 
    ciUseReadoutGroup->setVisible(_useGroups);

    setGroup(iNodeIndex, iNodeGroup);
    
#ifdef DBUG
    printf("Added node %s Group %d\n",qPrintable(node.plabel()), iNodeGroup);
#endif
    while (_groups.size()<=iNodeIndex)
      _groups.push_back(0);
    _groups[iNodeIndex] = ciUseReadoutGroup;
  }

  static_cast<QBoxLayout*>(_group->layout())
    ->insertLayout(index,layoutButton); 
    
  emit list_changed();
}

//Takes a QString of node names, Checks to see if they are still available
void NodeGroup::_build_notfoundlist(const QString& plabel) {

        int indexNotfound = _notfound.indexOf(plabel);
	if (indexNotfound != -1) {
	  _notfound.removeAt(indexNotfound);
	} 

        QString changing_notfound;        
        for(int k =0; k <_notfound.size();  k++){
          changing_notfound.append(QString("Missing: %1\n").arg(_notfound.at(k)));
	} 
        _notfoundlist->setText(changing_notfound);
}


void NodeGroup::replace_node(int index)
{ 
  const NodeSelect& node = _nodes[index];
  QCheckBox* button = static_cast<QCheckBox*>(_buttons->button(index));
  if (node.alias().size()) {
    button->setText(node.alias());
    button->setToolTip(node.label());
  }
  else
    button->setText(node.label());
  button->setPalette( isReady(index) ? *_ready : *_notready );

  emit list_changed();
}

QList<Node> NodeGroup::selected() 
{
  _persist.clear();
  _persistGroup.clear();
  _persistTrans.clear();

  _persistSelect.clear();

  QList<Node> nodes;
  QList<QAbstractButton*> buttons = _buttons->buttons();
  Node* master_evr = 0;
  foreach(QAbstractButton* b, buttons) {
    int id = _buttons->id(b);
    setGroup(id, _useGroups ? _groups[id]->currentText().toUInt(): 1);
    if (_useTransient)
      setTransient(id, _transients[id]->currentIndex()==1);

    _persist.push_back(_nodes[id].plabel().replace('\n','\t'));
    _persistGroup.push_back(_groups[id]->currentText().toUInt());
    _persistTrans.push_back(_transients[id]->currentIndex()!=0);
    _persistSelect.push_back(b->isChecked());

    if (b->isChecked()) {    
      if (_nodes[id].det().device()==DetInfo::Evr) {
        if (_nodes[id].det().devId()==0)
          master_evr = new Node(_nodes[id].node());
        else
          nodes.push_front(_nodes[id].node());
      }
      else {
        foreach(const NodeSelect& n, expanded(id))
          nodes << n.node();
      }
    }
  }

  if (master_evr) {
    nodes.push_front(*master_evr);
    delete master_evr;
  }

  //  Write persistent selected nodes
  Preferences pref(qPrintable(title()), _platform, "w");
  for (int iNode = 0; iNode < _persist.size(); ++iNode) {
      pref.write(_persist[iNode],
		 _persistGroup[iNode], 
		 _persistTrans[iNode]?cTransient:cRecord,
		 _persistSelect[iNode]?cSelected:cNotSelected);
  }

  _write_pref();

  return nodes;

}

std::set<std::string> NodeGroup::deviceNames()
{
  std::set<std::string> rv;   // return value
  QList<QAbstractButton*> buttons = _buttons->buttons();
  foreach(QAbstractButton* b, buttons) {
    if (b->isChecked()) {
      int id = _buttons->id(b);
      foreach(const NodeSelect& n, expanded(id)) {
        foreach(std::string sss, n.deviceNames()) {
          rv.insert(sss);
        }
      }
    }
  }
  return rv;
}

NodeGroup* NodeGroup::freeze()
{

  for(int k =0; k <_notfound.size();  k++){
      printf("ERROR NOT FOUND: %s\n", qPrintable(_notfound[k]));
      } 

  NodeGroup* g = new NodeGroup(title(),(QWidget*)this, _platform, _useGroups, _useTransient);

  {
    QList<QAbstractButton*> buttons = _buttons->buttons();
    foreach(QAbstractButton* b, buttons) {
      if (b->isChecked())
        g->addNode(_nodes[_buttons->id(b)]);
    }
  }

  //{
  //  QList<QAbstractButton*> buttons = g->_buttons->buttons();
  //  foreach(QAbstractButton* b, buttons) {
  //    b->setEnabled(false);
  //  }
  //}  
  g->setEnabled(false);  
  return g;
}

bool NodeGroup::ready() const
{
  QList<QAbstractButton*> buttons = _buttons->buttons();
  QList<QString> require = _require;
  int index;
  for(int i=0; i<buttons.size(); i++) {
    if (buttons[i]->isChecked() && !isReady(i))
      return false;
    if ((index=require.indexOf(_nodes[i].plabel()))>=0)
      require.removeAt(index);
  }
  return require.empty();
}

Node& NodeGroup::node(int i)
{
  return (Node&)_nodes[i].node();
}

QList<NodeSelect> NodeGroup::expanded(int i)
{
  QList<NodeSelect> nodes;
  const QList<NodeSelect>& children = _children[i];
  if (children.isEmpty()) {
    nodes << _nodes[i].node();
  } else {
    foreach(const NodeSelect& child, children) {
      nodes << child;
    }
  }

  return nodes;
}

bool NodeGroup::isReady(int i) const
{
  const QList<NodeSelect>& children = _children[i];
  bool ready = children.isEmpty() ? _nodes[i].ready() : true;

  foreach(const NodeSelect& child, children) {
    if (!child.ready()) {
      ready = false;
      break;
    }
  }

  return ready;
}

void NodeGroup::setGroup(int parent, uint16_t group)
{
  QList<NodeSelect>::iterator it;
  QList<NodeSelect>& children = _children[parent];
  _nodes[parent].node().setGroup(group);
  for(it = children.begin(); it != children.end(); ++it) {
    it->node().setGroup(group);
  }
}

void NodeGroup::setTransient(int parent, bool t)
{
  QList<NodeSelect>::iterator it;
  QList<NodeSelect>& children = _children[parent];
  _nodes[parent].node().setTransient(t);
  for(it = children.begin(); it != children.end(); ++it) {
    it->node().setTransient(t);
  }
}

void NodeGroup::addChild(int parent, const NodeSelect& child)
{
  QList<NodeSelect>& children = _children[parent];
  int index = children.indexOf(child);
  if (index >= 0) {
    children.replace(index, child);
  } else {
    children << child;
  }
}

void NodeGroup::_read_pref(const QString&  title,
                           QList<QString>& l,
                           QList<bool>&    lt)
{
  Preferences pref(qPrintable(title), _platform, "r");
  pref.read(l,lt,cTransient);
}


void NodeGroup::_read_pref(const QString&  title,
                           QList<QString>& l, 
                           QList<int>&     lg,
			   QList<bool>&    lt)
{
  Preferences pref(qPrintable(title), _platform, "r");
  pref.read(l,lg,lt,cTransient);
}

void NodeGroup::_read_pref(const QString&  title,
                           QList<QString>& l, 
                           QList<int>&     lg,
			   QList<bool>&    lt,
			   QList<bool>&    ls)
{
  Preferences pref(qPrintable(title), _platform, "r");
  pref.read(l,lg,lt,cTransient,ls,cSelected);
}


int NodeGroup::order(const NodeSelect& node, const QString& text)
{
  QBoxLayout* l = static_cast<QBoxLayout*>(_group->layout());
  for(int index = 0; index < l->count(); index++) {
    QLayout* h = l->itemAt(index)->layout();
    QString sLabel = static_cast<QCheckBox*>(static_cast<QHBoxLayout*>(h)
					     ->itemAt(0)->widget()) ->text();
    if (text < sLabel)
      return index;
  }
  return l->count();
}

NodeTransientCb::NodeTransientCb(NodeGroup& nodeGroup, int iNodeIndex, QWidget& 
button, bool sel) :
  _nodeGroup(nodeGroup), _iNodeIndex(iNodeIndex), _button(button), _selected(sel)
{
}

void NodeTransientCb::selectChanged(bool v)
{
  _selected = v;
  if (v)
    _button.setPalette(_nodeGroup.node(_iNodeIndex).transient() ? QPalette(Qt::yellow) : QPalette(Qt::green));
  else
    _button.setPalette(QPalette());
}

void NodeTransientCb::stateChanged(int i)
{
  _nodeGroup.setTransient(_iNodeIndex, i!=0);
  if (_selected)
    _button.setPalette(i!=0 ? QPalette(Qt::yellow) : QPalette(Qt::green));
}
