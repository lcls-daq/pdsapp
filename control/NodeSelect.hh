#ifndef Pds_NodeSelect_hh
#define Pds_NodeSelect_hh

#include <set>

#include <QtGui/QWidget>
#include <QtGui/QCheckBox>
#include <QtCore/QList>
#include <QtCore/QString>

#include "pds/collection/Node.hh"
#include "pdsdata/xtc/DetInfo.hh"
#include "pdsdata/xtc/BldInfo.hh"

class QGroupBox;
class QButtonGroup;
class QPalette;

namespace Pds {
  class PingReply;

  class NodeSelect {
  public:
    NodeSelect(const Node& node);
    NodeSelect(const Node& node, const PingReply& msg);
    NodeSelect(const Node& node, const char* desc);
    NodeSelect(const Node& node, const BldInfo& info);
    NodeSelect(const NodeSelect&);
    ~NodeSelect();
  public:
    const QString& label() const { return _label; }
    const Node&    node () const { return _node; }
    const DetInfo& det  () const { return static_cast<const DetInfo&>(_src); }
    const std::set<std::string>& deviceNames() const { return _deviceNames; }
    const BldInfo& bld  () const { return static_cast<const BldInfo&>(_src); }
    const Src&     src  () const { return _src; }
    bool           ready() const { return _ready; }
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
  };
  
  class CallbackNodeGroup;

  class NodeGroup : public QWidget {
    Q_OBJECT
  public:
    NodeGroup(const QString& label, QWidget* parent, unsigned platform, int iUseReadoutGroup = 0);
    ~NodeGroup();
  public slots:
    void add_node    (int);
    void replace_node(int);
  signals:
    void node_added   (int);
    void node_replaced(int);
    void list_changed ();
  public:
    void addNode(const NodeSelect&);
    unsigned       nodes() const;
    QList<Node>    selected();
    QList<DetInfo> detectors();
    std::set<std::string>  deviceNames();
    QList<BldInfo> reporters();
    NodeGroup*     freeze  ();
    bool           ready   () const;
    void           setGroup(int iNodeIndex, int iGroup);    
  private:
    void _read_pref(const QString&, QList<QString>&, QList<int>&);
  private:
    QGroupBox*     _group;
    QButtonGroup*  _buttons;
    QList<NodeSelect> _nodes;
    QList<QString> _persist;
    QList<int>     _persistGroup;
    QList<QString> _require;
    QList<int>     _requireGroup;
    QList<int>     _order;
    QPalette*      _ready;
    QPalette*      _notready;
    unsigned       _platform;
    /*
     * _iUseReadoutGroup : 0: No readout group, 1: Readout group UI, 2: Default readout group without UI
     */
    int            _iUseReadoutGroup;
    QList<CallbackNodeGroup*>
                   _lCallback;
  };

  class CallbackNodeGroup : public QObject
  {
    Q_OBJECT
  public:
    CallbackNodeGroup(NodeGroup& nodeGroup, int iNodeIndex);
    
  public slots:    
    void currentIndexChanged(int iGroupIndex);
    
  private:
    NodeGroup& _nodeGroup;
    int        _iNodeIndex;
  };

}; // namespace Pds

#endif
