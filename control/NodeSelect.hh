#ifndef Pds_NodeSelect_hh
#define Pds_NodeSelect_hh

#include <set>

#include <QtGui/QGroupBox>
#include <QtGui/QCheckBox>
#include <QtCore/QList>
#include <QtCore/QString>

#include "pds/collection/Node.hh"
#include "pdsdata/xtc/DetInfo.hh"
#include "pdsdata/xtc/BldInfo.hh"

class QButtonGroup;
class QPalette;

namespace Pds {
  class AliasReply;
  class IocNode;

  class NodeSelect {
  public:
    NodeSelect(const Node& node);
    NodeSelect(const Node& node, 
	       bool ready,
	       const std::vector<Src> sources,
	       QString alias="");
    NodeSelect(const Node& node, const char* desc);
    NodeSelect(const Node& node, const BldInfo& info);
    NodeSelect(const IocNode& node);
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
  
  class CallbackNodeGroup;
  class NodeTransientCb;

  class NodeGroup : public QWidget {
    Q_OBJECT
  public:
    NodeGroup(const QString& label, 
	      QWidget*       parent, 
	      unsigned       platform, 
	      int            iUseReadoutGroup = 0, 
	      bool           useTransient=false);
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
    QString        title() const;
    unsigned       nodes() const;
    QList<Node>    selected();
    QList<DetInfo> detectors();
    std::set<std::string>  deviceNames();
    QList<BldInfo> reporters();
    QList<BldInfo> transients();
    QList<DetInfo> iocs      ();
    NodeGroup*     freeze  ();
    bool           ready   () const;
  private:
    void _read_pref(const QString&, QList<QString>&, QList<bool>&);
    void _read_pref(const QString&, QList<QString>&, QList<int>&, QList<bool>&);
  private:
    QGroupBox*     _group;
    QButtonGroup*  _buttons;
    QList<NodeSelect> _nodes;
    QList<QString> _persist;
    QList<int>     _persistGroup;
    QList<bool>    _persistTrans;
    QList<QString> _require;
    QList<int>     _requireGroup;
    QList<bool>    _requireTrans;
    QList<int>     _order;
    QPalette*      _ready;
    QPalette*      _notready;
    QPalette*      _warn;
    unsigned       _platform;
  private:
    Node& node(int);

    /*
     * _iUseReadoutGroup : 0: No readout group, 1: Readout group UI, 2: Default readout group without UI
     */
    int            _iUseReadoutGroup;
    QList<CallbackNodeGroup*>
                   _lCallback;

    bool                    _useTransient;
    QList<NodeTransientCb*> _lTransientCb;

    friend class CallbackNodeGroup;
    friend class NodeTransientCb;
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

  class NodeTransientCb : public QObject
  {
    Q_OBJECT
  public:
    NodeTransientCb(NodeGroup& nodeGroup, int iNodeIndex, QWidget& button, bool selected);
  public slots:    
    void selectChanged(bool);
    void stateChanged(int);
  private:
    NodeGroup& _nodeGroup;
    int        _iNodeIndex;
    QWidget&   _button;
    bool       _selected;
  };

}; // namespace Pds

#endif
