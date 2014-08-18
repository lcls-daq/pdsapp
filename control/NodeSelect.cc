#include "pdsapp/control/NodeSelect.hh"

#include "pdsdata/xtc/BldInfo.hh"
#include "pdsdata/xtc/DetInfo.hh"
#include "pds/ioc/IocNode.hh"

#include <QtCore/QString>

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>

using namespace Pds;

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

NodeSelect::NodeSelect(const Node& node, 
		       bool ready,
		       const std::vector<Src> sources,
		       QString alias) :
  _node    (node),
  _ready   (ready),
  _alias   (alias)
{
  if (sources.size()) {
    bool found=false;
    for(unsigned i=0; i<sources.size(); i++) {
      if (sources[i].level()==Level::Source) {
        const DetInfo& src = static_cast<const DetInfo&>(sources[i]);
        _deviceNames.insert(DetInfo::name(src));
        if (!found) {
          found=true;
          _src    = sources[i];
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
  _alias = QString("");
}

NodeSelect::NodeSelect(const Node& node, const BldInfo& info) :
  _node  (node),
  _src   (info),
  _label (BldInfo::name(info)),
  _ready (true),
  _alias (QString(""))
{
}

NodeSelect::NodeSelect(const IocNode& node) :
  _node  (node.node()),
  _src   (node.src()),
  _label (node.alias()),
  _ready (true),
  _alias (QString(node.alias()))
{
}

NodeSelect::NodeSelect(const NodeSelect& s) :
  _node (s._node),
  _src  (s._src ),
  _deviceNames(s._deviceNames),
  _label(s._label),
  _ready(s._ready),
  _alias(s._alias)
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

const DetInfo& NodeSelect::det  () const 
{ return static_cast<const DetInfo&>(_src); }

const BldInfo& NodeSelect::bld  () const 
{ return static_cast<const BldInfo&>(_src); }
