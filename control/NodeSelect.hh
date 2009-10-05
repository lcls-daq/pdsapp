#ifndef Pds_NodeSelect_hh
#define Pds_NodeSelect_hh

#include <QtGui/QGroupBox>
#include <QtGui/QCheckBox>
#include <QtCore/QList>

#include "pds/collection/Node.hh"

class QButtonGroup;

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
    const Node& node() const { return _node; }
  public:
    bool operator==(const NodeSelect&) const;
  private:
    Node _node;
    QString _label;
  };

  class NodeGroup : public QGroupBox {
    Q_OBJECT
  public:
    NodeGroup(const QString& label, QWidget* parent);
    ~NodeGroup();
  public slots:
    void add_node(int);
  signals:
    void node_added(int);
  public:
    void addNode(const NodeSelect&);
    QList<Node> selected();
    NodeGroup*  freeze  ();
  private:
    QButtonGroup* _buttons;
    QList<NodeSelect> _nodes;
  };

};

#endif
