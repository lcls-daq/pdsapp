#ifndef Pds_NodeGroup_hh
#define Pds_NodeGroup_hh

#include "pdsapp/control/NodeSelect.hh"
#include "pds/collection/Node.hh"

#include <QtCore/QList>
#include <QtCore/QString>
#include <QtGui/QWidget>

#include <set>

class QButtonGroup;
class QGroupBox;
class QPalette;
class QCheckBox;
class QComboBox;

namespace Pds {
  class NodeTransientCb;
  class Preference;
  
  class NodeGroup : public QWidget {
    Q_OBJECT
  public:
    NodeGroup(const QString& label, 
	      QWidget*       parent, 
	      unsigned       platform, 
	      int            useGroups=0,
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
    std::set<std::string>  deviceNames();
    NodeGroup*     freeze  ();
    bool           ready   () const;
  protected:
    virtual int    order     (const NodeSelect&, const QString&);
  private:
    virtual void   _write_pref() const {};
  private:
    void _read_pref(const QString&, QList<QString>&, QList<bool>&);
    void _read_pref(const QString&, QList<QString>&, QList<int>&, QList<bool>&);
  protected:
    QGroupBox*     _group;
    QButtonGroup*  _buttons;
    QList<NodeSelect> _nodes;
    QList<QString> _persist;
    QList<int>     _persistGroup;
    QList<bool>    _persistTrans;
    QList<QString> _require;
    QList<int>     _requireGroup;
    QList<bool>    _requireTrans;
    QPalette*      _ready;
    QPalette*      _notready;
    QPalette*      _warn;
    unsigned       _platform;
  private:
    Node& node(int);

    int               _useGroups;
    QCheckBox*        _enableGroups;
    QList<QComboBox*> _groups;

    bool              _useTransient;
    QList<QComboBox*> _transients;
    QList<NodeTransientCb*> _lTransientCb;

    friend class CallbackNodeGroup;
    friend class NodeTransientCb;
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
