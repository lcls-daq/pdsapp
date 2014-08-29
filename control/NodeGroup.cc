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

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

using namespace Pds;

static const char* cTransient = "Monitor";
static const char* cRecord    = "Record";

NodeGroup::NodeGroup(const QString& label, QWidget* parent, unsigned platform, int useGroups, bool useTransient) :
  QWidget  (parent),
  _group   (new QGroupBox(label)),
  _buttons (new QButtonGroup(parent)),
  _ready   (new QPalette(Qt::green)),
  _notready(new QPalette(Qt::red)),
  _platform(platform),
  _useGroups(useGroups),
  _useTransient    (useTransient)
{
  //  Read persistent selected nodes
  if (useGroups<=0) {
    _read_pref(title(), 
	       _persist, _persistTrans);
    _read_pref(QString("%1 required").arg(title()), 
	       _require, _requireTrans);
  }
  else {
    if (_useGroups>=2) _useGroups=2;
    _read_pref(title(), 
	       _persist, _persistGroup, _persistTrans);
   _read_pref(QString("%1 required").arg(title()), 
	       _require, _requireGroup, _requireTrans);
  }    

  _buttons->setExclusive(false);

  _group->setLayout(new QVBoxLayout);
  QVBoxLayout* l = new QVBoxLayout;
  l->addWidget(_group);
  if (_useGroups) {
    l->addWidget(_enableGroups=new QCheckBox("Use Readout Groups"));
    _enableGroups->setChecked(_useGroups==2);
    _enableGroups->setEnabled(false);
  }
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
    this->node(index).setGroup    (old.node().group());
    this->node(index).setTransient(old.node().transient());
    emit node_replaced(index);
  }
  else {
    index = _nodes.size();
    _nodes << node;
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
  button->setCheckState( (indexPersist>=0 || indexRequire>=0) ? Qt::Checked : Qt::Unchecked);
  button->setEnabled   ( indexRequire<0 );
  button->setPalette( node.ready() ? *_ready : *_notready );
  _buttons->addButton(button,index); 
  QObject::connect(button, SIGNAL(clicked()), this, SIGNAL(list_changed()));

  index = order(node,button->text());

  QHBoxLayout* layoutButton = new QHBoxLayout;
  layoutButton->addWidget(button); 

  bool bEvrNode = (node.src().level() == Level::Source &&
		   static_cast<const DetInfo&>(node.src()).device() == DetInfo::Evr);          

  if (_useTransient) {

    QComboBox* transientBox = new QComboBox(this);
    transientBox->addItem("Rec");
    transientBox->addItem("Mon");
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
    
  if (_useGroups)
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
    ciUseReadoutGroup->setVisible(_enableGroups->isChecked());

    this->node(iNodeIndex).setGroup(iNodeGroup);    
    
    connect(_enableGroups, SIGNAL(toggled(bool)), ciUseReadoutGroup, SLOT(setVisible(bool)));
#ifdef DBUG
    printf("Added node %s Group %d\n",qPrintable(node.plabel()), iNodeGroup);
#endif
    while (_groups.size()<=iNodeIndex)
      _groups.push_back(0);
    _groups[iNodeIndex] = ciUseReadoutGroup;
  }
  else
  {
#ifdef DBUG
    printf("Added node %s\n",qPrintable(node.plabel()));
#endif
  }    

  static_cast<QBoxLayout*>(_group->layout())
    ->insertLayout(index,layoutButton); 
    
  emit list_changed();
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
  button->setPalette( node.ready() ? *_ready : *_notready );

  emit list_changed();
}

QList<Node> NodeGroup::selected() 
{
  _persist.clear();
  _persistGroup.clear();
  _persistTrans.clear();
  QList<Node> nodes;
  QList<QAbstractButton*> buttons = _buttons->buttons();
  Node* master_evr = 0;
  foreach(QAbstractButton* b, buttons) {
    int id = _buttons->id(b);
    if (_useGroups && _enableGroups->isChecked())
      _nodes[id].node().setGroup(_groups[id]->currentText().toUInt());
    if (_useTransient)
      _nodes[id].node().setTransient(_transients[id]->currentIndex()==1);

    if (b->isChecked()) {
      _persist.push_back(_nodes[id].plabel().replace('\n','\t'));
      _persistGroup.push_back(_useGroups ?_groups[id]->currentText().toUInt():0);
      _persistTrans.push_back(_useTransient ?_transients[id]->currentIndex()!=0:0);
      
      //printf("node %d %s group %d\n", id, qPrintable(_persist[id]), _persistGroup[id]); //!!!debug
      
      if (_nodes[id].det().device()==DetInfo::Evr) {
        if (_nodes[id].det().devId()==0)
          master_evr = new Node(_nodes[id].node());
        else
          nodes.push_front(_nodes[id].node());
      }
      else
        nodes.push_back(_nodes[id].node());
    }
  }

  if (master_evr) {
    nodes.push_front(*master_evr);
    delete master_evr;
  }

  //  Write persistent selected nodes
  Preferences pref(qPrintable(title()), _platform, "w");
  for (int iNode = 0; iNode < _persist.size(); ++iNode) {
    if (_useGroups)
      pref.write(_persist[iNode],
		 _persistGroup[iNode], 
		 _persistTrans[iNode]?cTransient:cRecord);
    else if (_useTransient)
      pref.write(_persist[iNode],
		 _persistTrans[iNode]?cTransient:cRecord);
    else
      pref.write(_persist[iNode]);      
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
      foreach(std::string sss, _nodes[id].deviceNames()) {
        rv.insert(sss);
      }
    }
  }
  return rv;
}

NodeGroup* NodeGroup::freeze()
{
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
    if (buttons[i]->isChecked() && !_nodes[i].ready())
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
 _nodeGroup.node(_iNodeIndex).setTransient(i!=0);
  if (_selected)
    _button.setPalette(i!=0 ? QPalette(Qt::yellow) : QPalette(Qt::green));
}
