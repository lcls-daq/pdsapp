#ifndef Pds_SelectDialog_hh
#define Pds_SelectDialog_hh

#include <set>
#include <vector>
#include <string>

#include "pds/management/PlatformCallback.hh"
#include "pds/ioc/IocHostCallback.hh"

#include <QtGui/QDialog>

#include "pds/collection/Node.hh"
#include "pdsdata/xtc/DetInfo.hh"
#include "pdsdata/xtc/BldInfo.hh"
#include <QtCore/QList>
#include <QtCore/QString>
#include <QtGui/QHBoxLayout>
#include <QtGui/QVBoxLayout>
#include <QtGui/QPushButton>

namespace Pds {
  class PartitionControl;
  class IocControl;
  class NodeGroup;

  class NodeMap {
  public:
    NodeMap(const Node& n, const std::vector<Src>& s) :
      node(n), sources(s) {}
  public:
    Node node;
    std::vector<Src> sources;
  };

  class SelectDialog : public QDialog,
		       public PlatformCallback,
		       public IocHostCallback {
    Q_OBJECT
  public:
    SelectDialog(QWidget* parent,
		 PartitionControl& control, 
		 IocControl& icontrol,
		 bool bReadGroupEnable,
		 bool autorun=false);
    ~SelectDialog();
  public:
    void        available(const Node& hdr, const PingReply& msg);
    void        connected(const IocNode&);
    void        aliasCollect(const Node& hdr, const AliasReply& msg);
    bool        aliasLookup(const Src& det, QString& alias);
    const QList<Node    >& selected()  const;
    const QList<DetInfo >& detectors() const;
    const std::set<std::string>& deviceNames() const;
    const QList<ProcInfo>& segments () const;
    const QList<BldInfo >& reporters() const;
    const QList<BldInfo >& transients() const;
    const QList<DetInfo >& iocs      () const;
    const std::list<NodeMap>& segment_map() const;
    QWidget*               display ();
  public:
    static void useTransient(bool);
  public slots:
    void select();
    void check_ready();
    void update_layout();
  signals:
    void changed();
  private:
    void _clearLayout();
  private:
    PartitionControl& _pcontrol;
    IocControl&       _icontrol;
    bool              _bReadGroupEnable;
    bool              _autorun;
    Node _control;
    NodeGroup* _segbox;
    NodeGroup* _evtbox;
    NodeGroup* _rptbox;
    NodeGroup* _iocbox;
    QList<Node>     _selected;
    QList<DetInfo > _detinfo;
    std::set<std::string> _deviceNames;
    QList<ProcInfo> _seginfo;
    QList<BldInfo > _rptinfo;
    QList<BldInfo > _trninfo;
    QList<DetInfo > _iocinfo;
    QPushButton*   _acceptb;
    std::list<NodeMap> _segment_map;
  };
};

#endif
