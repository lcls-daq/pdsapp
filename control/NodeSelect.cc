#include "NodeSelect.hh"

#include <errno.h>
#include <QtCore/QString>
#include <QtGui/QButtonGroup>
#include <QtGui/QVBoxLayout>
#include <QtGui/QPalette>
#include <QtGui/QComboBox>

#include "pdsdata/xtc/BldInfo.hh"
#include "pdsdata/xtc/DetInfo.hh"
#include "pds/collection/PingReply.hh"
#include "pds/config/EvrConfigType.hh"

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
         << BldInfo::HfxMonImb03
         << BldInfo::HfxMonCam
         << BldInfo::HfxDg3Imb01
         << BldInfo::HfxDg3Imb02
         << BldInfo::HfxDg3Cam
         << BldInfo::XcsDg3Imb03
         << BldInfo::XcsDg3Imb04
         << BldInfo::XcsDg3Cam
         << BldInfo::MecLasEm01
         << BldInfo::MecTctrPip01
         << BldInfo::MecTcTrDio01
         << BldInfo::MecXt2Ipm02
         << BldInfo::MecXt2Ipm03
         << BldInfo::MecHxmIpm01
         << BldInfo::GMD
         << BldInfo::CxiDg1Imb01
         << BldInfo::CxiDg2Imb01
         << BldInfo::CxiDg2Imb02
         << BldInfo::CxiDg4Imb01
         << BldInfo::CxiDg1Pim
         << BldInfo::CxiDg2Pim
         << BldInfo::CxiDg4Pim
         << BldInfo::NumberOf;

static FILE* open_pref(const char* title, unsigned platform, char* mode)
{
  const int BUFF_SIZE=256;
  char* buff = new char[BUFF_SIZE];

  char* home = getenv("HOME");
  if (home) {
    snprintf(buff, NODE_BUFF_SIZE-1, "%s/.%s for platform %u", home, title, platform);
  }
  else {
    snprintf(buff, NODE_BUFF_SIZE-1, ".%s for platform %u", title, platform);
  }
  
  FILE* f = fopen(buff,mode);
  if (!f) {
    printf("Failed to open %s\n", buff);
  }
  delete[] buff;

  return f;
}

NodeGroup::NodeGroup(const QString& label, QWidget* parent, unsigned platform, int iUseReadoutGroup) :
#ifdef BALANCE_LAYOUT
  QWidget  (parent),
  _group   (new QGroupBox(label)),
#else
  QGroupBox(label, parent),
#endif
  _buttons (new QButtonGroup(parent)),
  _ready   (new QPalette(Qt::green)),
  _notready(new QPalette(Qt::red)),
  _platform(platform),
  _iUseReadoutGroup(iUseReadoutGroup)
{
  if ( _iUseReadoutGroup < 0 )
    _iUseReadoutGroup = 0;
  else if ( _iUseReadoutGroup > 2 )
    _iUseReadoutGroup = 2;
    
  //  Read persistent selected nodes
  _read_pref(title(), _persist, _persistGroup);
  _read_pref(QString("%1 required").arg(title()), _require, _requireGroup);

  _buttons->setExclusive(false);

#ifdef BALANCE_LAYOUT
  _group->setLayout(new QVBoxLayout);
  QVBoxLayout* l = new QVBoxLayout;
  l->addWidget(_group);
  l->addStretch();
  setLayout(l);
#else
  setLayout(new QVBoxLayout(this));
#endif

  connect(this, SIGNAL(node_added(int)), 
          this, SLOT(add_node(int)));
  connect(this, SIGNAL(node_replaced(int)), 
          this, SLOT(replace_node(int)));
}

NodeGroup::~NodeGroup() 
{
  delete _buttons; 
  
  CallbackNodeGroup* pCallback;
  foreach (pCallback, _lCallback)
    delete pCallback;
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

#ifdef BALANCE_LAYOUT
unsigned NodeGroup::nodes() const
{
  return _nodes.size();
}

QString NodeGroup::title() const
{
  return _group->title();
}
#endif

void NodeGroup::add_node(int index)
{ 
  const int iNodeIndex = index;
  const NodeSelect& node = _nodes[index];
  QCheckBox* button = new QCheckBox(node.label(),this);
    
  int indexPersist = _persist.indexOf(node.plabel());
  int indexRequire = _require.indexOf(node.plabel());
  button->setCheckState( (indexPersist>=0 || indexRequire>=0) ? Qt::Checked : Qt::Unchecked);
  button->setEnabled   ( indexRequire<0 );
  button->setPalette( node.ready() ? *_ready : *_notready );
  _buttons->addButton(button,index); 
  QObject::connect(button, SIGNAL(clicked()), this, SIGNAL(list_changed()));
  
#ifdef BALANCE_LAYOUT
  QBoxLayout* l = static_cast<QBoxLayout*>(_group->layout());
#else
  QBoxLayout* l = static_cast<QBoxLayout*>(layout());
#endif
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
      QString sLabel;
      if (_iUseReadoutGroup == 1)
        sLabel = static_cast<QCheckBox*>(
                    static_cast<QHBoxLayout*>(l->itemAt(index)->layout())
                   ->itemAt(0)->widget()) ->text();
      else
        sLabel = static_cast<QCheckBox*>(l->itemAt(index)->widget())->text();
      //if (node.label() < static_cast<QCheckBox*>(l->itemAt(index)->widget())->text())
      if (node.label() < sLabel)
        break;
    }
  }
  
  //!!! readout group
  if (_iUseReadoutGroup == 2)
  {
    bool bEvrNode = ( 
      node.src().level() == Level::Source &&
      static_cast<const DetInfo&>(node.src()).device() == DetInfo::Evr);          
    int iNodeGroup = (bEvrNode? 0:1);
    setGroup(iNodeIndex, iNodeGroup);          
    
    l->insertWidget(index,button); 
    printf("Added node %s Group (Default) %d\n",qPrintable(node.plabel()), iNodeGroup);
  }
  else
  if (_iUseReadoutGroup == 1)
  {    
        
    QHBoxLayout* layoutButton = new QHBoxLayout;
    layoutButton->addWidget(button); 
    
    QComboBox* ciUseReadoutGroup = new QComboBox(this);
            
    bool bEvrNode = ( 
      node.src().level() == Level::Source &&
      static_cast<const DetInfo&>(node.src()).device() == DetInfo::Evr);                
    int iNodeGroup;
    if ( bEvrNode )
    {
      iNodeGroup = 0;
      ciUseReadoutGroup->addItem(QString().setNum(0));
      ciUseReadoutGroup->setCurrentIndex(0);    
      ciUseReadoutGroup->setDisabled(true);
    }
    else
    {
      iNodeGroup = ( indexPersist >= 0 ? _persistGroup[indexPersist] : 1 );            
      for (int iGroup=1; iGroup <= EvrConfigType::EventCodeType::MaxReadoutGroup; ++iGroup)
        ciUseReadoutGroup->addItem(QString().setNum(iGroup));
      ciUseReadoutGroup->setCurrentIndex( iNodeGroup-1 );    
    }
        
    layoutButton->addStretch(1);         
    layoutButton->addWidget(ciUseReadoutGroup); 
    
    setGroup(iNodeIndex, iNodeGroup);    
    
    _lCallback.append(new CallbackNodeGroup(*this, iNodeIndex));
    connect(ciUseReadoutGroup, SIGNAL(currentIndexChanged(int)), _lCallback.last(), SLOT(currentIndexChanged(int)));    
    
    l->insertLayout(index,layoutButton);   
    printf("Added node %s Group %d\n",qPrintable(node.plabel()), iNodeGroup);
  }
  else
  {
    l->insertWidget(index,button); 
    printf("Added node %s\n",qPrintable(node.plabel()));
  }    
  
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
  _persistGroup.clear();
  QList<Node> nodes;
  QList<QAbstractButton*> buttons = _buttons->buttons();
  Node* master_evr = 0;
  foreach(QAbstractButton* b, buttons) {
    if (b->isChecked()) {
      int id = _buttons->id(b);
      _persist.push_back(_nodes[id].plabel().replace('\n','\t'));
      _persistGroup.push_back(_nodes[id].node().group());
      
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
  FILE* f = open_pref(qPrintable(title()), _platform,"w");
  if (f) {
    for (int iNode = 0; iNode < _persist.size(); ++iNode)
    {
      //foreach(QString p, _persist) {      
        //fprintf(f,"%s;%d\n",qPrintable(p), _persistGroup);
        
      if (_iUseReadoutGroup != 0)
        fprintf(f,"%s;%d\n",qPrintable(_persist[iNode]), _persistGroup[iNode]);
      else
        fprintf(f,"%s\n",qPrintable(_persist[iNode]));      
    }
    fclose(f);
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
  NodeGroup* g = new NodeGroup(title(),(QWidget*)this, _platform, _iUseReadoutGroup);

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
  for(int i=0; i<buttons.size(); i++)
    if (buttons[i]->isChecked() && !_nodes[i].ready())
      return false;
  return true;
}

void NodeGroup::setGroup(int iNodeIndex, int iGroup)
{
  //printf("set node %d group %d\n", iNodeIndex, iGroup);//!!! debug
  ((Node&) _nodes[iNodeIndex].node()).setGroup(iGroup);
}

void NodeGroup::_read_pref(const QString&  title,
                           QList<QString>& l, 
                           QList<int>&     lg)
{
  char *buff = (char *)malloc(NODE_BUFF_SIZE);  // use malloc w/ getline
  if (buff == (char *)NULL) {
    printf("%s: malloc(%d) failed, errno=%d\n", __PRETTY_FUNCTION__, NODE_BUFF_SIZE, errno);
    return;
  }
    
  FILE* f = open_pref(qPrintable(title), _platform, "r");
  if (f) {
    char* lptr=buff;
    unsigned linesz = NODE_BUFF_SIZE;         // initialize for getline
    
    const char* lsReadGroupDesc[] = 
      { "", "with Readout Group (UI)", "with default Readout Group (No UI)" };
    printf("Reading pref file \"%s\" %s\n", qPrintable(title), lsReadGroupDesc[_iUseReadoutGroup]);      
    while(getline(&lptr,&linesz,f)!=-1) {
      QString p(lptr);
      p.chop(1);  // remove new-line
      p.replace('\t','\n');        
      
      if (_iUseReadoutGroup != 0)
      {          
        QStringList ls = p.split(';');                    
        if (ls.size()>0)
        {
          int iGroup = ( ls.size() <= 1 ? 0 : ls[1].toInt() );            
          l .push_back(ls[0]);
          lg.push_back(iGroup);
          
          printf("Persist %s Group %d\n",qPrintable(ls[0]), iGroup);
        }        
      }
      else
      {
        l.push_back(p);
        printf("Persist %s\n",qPrintable(p));
      }
    }
    fclose(f);
  }
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
        _deviceNames.insert(DetInfo::name(src));
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

  ////!!! for support group
  //_label += QString(" G %1").arg(node.group());      
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
  _deviceNames(s._deviceNames),
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

CallbackNodeGroup::CallbackNodeGroup(NodeGroup& nodeGroup, int iNodeIndex) :
  _nodeGroup(nodeGroup), _iNodeIndex(iNodeIndex)
{
}

void CallbackNodeGroup::currentIndexChanged(int iGroupIndex)
{
  _nodeGroup.setGroup(_iNodeIndex, iGroupIndex + 1);
}

