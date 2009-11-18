#ifndef Pds_NodeSelect_hh
#define Pds_NodeSelect_hh

#include <QtGui/QGroupBox>
#include <QtGui/QCheckBox>
#include <QtCore/QList>

#include "pds/collection/Node.hh"
#include "pdsdata/xtc/DetInfo.hh"

class QButtonGroup;
class QPalette;

namespace Pds {
  class PingReply;

  class NodeSelect {
  public:
    NodeSelect(const Node& node);
    NodeSelect(const Node& node, const PingReply& msg);
    NodeSelect(const Node& node, const char* desc);
    NodeSelect(const NodeSelect&);
    ~NodeSelect();
  public:
    const QString& label() const { return _label; }
    const Node&    node () const { return _node; }
    const DetInfo& det  () const { return _det; }
    bool           ready() const { return _ready; }
  public:
    bool operator==(const NodeSelect&) const;
  private:
    Node    _node;
    DetInfo _det;
    QString _label;
    bool    _ready;
  };

  class NodeGroup : public QGroupBox {
    Q_OBJECT
  public:
    NodeGroup(const QString& label, QWidget* parent);
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
    QList<Node>    selected();
    QList<DetInfo> detectors();
    NodeGroup*     freeze  ();
    bool           ready   () const;
  private:
    QButtonGroup* _buttons;
    QList<NodeSelect> _nodes;
    QPalette*     _ready;
    QPalette*     _notready;
  };

};

#endif
