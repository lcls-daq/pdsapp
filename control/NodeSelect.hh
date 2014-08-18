#ifndef Pds_NodeSelect_hh
#define Pds_NodeSelect_hh

#include "pds/collection/Node.hh"
#include "pdsdata/xtc/Src.hh"

#include <set>
#include <vector>

#include <QtCore/QString>

namespace Pds {
  class BldInfo;
  class DetInfo;
  class IocNode;

  class NodeSelect {
  public:
    NodeSelect(const Node& node);
    NodeSelect(const Node& node, 
	       bool ready,
	       const std::vector<Pds::Src> sources,
	       QString alias="");
    NodeSelect(const Node& node, const char* desc);
    NodeSelect(const Node& node, const BldInfo& info);
    NodeSelect(const IocNode& node);
    NodeSelect(const NodeSelect&);
    ~NodeSelect();
  public:
    const QString& label() const { return _label; }
    const Node&    node () const { return _node; }
    Node&          node ()       { return _node; }
    const DetInfo& det  () const;
    const std::set<std::string>& deviceNames() const { return _deviceNames; }
    const BldInfo& bld  () const;
    const Src&     src  () const { return _src; }
    bool           ready() const { return _ready; }
    const QString& alias() const { return _alias; }
  public:  // persistence
    QString plabel() const;
  public:
    bool operator==(const NodeSelect&) const;
  private:
    Node    _node;
    Src     _src;
    std::set<std::string> _deviceNames;
    QString _label;
    bool    _ready;
    QString _alias;
  };
};

#endif
