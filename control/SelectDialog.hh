#ifndef Pds_SelectDialog_hh
#define Pds_SelectDialog_hh

#include "pds/management/PlatformCallback.hh"
#include <QtGui/QDialog>

#include "pds/collection/Node.hh"
#include "pdsdata/xtc/DetInfo.hh"
#include "pdsdata/xtc/BldInfo.hh"
#include <QtCore/QList>
#include <QtGui/QHBoxLayout>
#include <QtGui/QVBoxLayout>
#include <QtGui/QPushButton>

namespace Pds {
  class PartitionControl;
  class NodeGroup;

  class SelectDialog : public QDialog,
		       public PlatformCallback {
    Q_OBJECT
  public:
    SelectDialog(QWidget* parent,
		 PartitionControl& control);
    ~SelectDialog();
  public:
    void        available(const Node& hdr, const PingReply& msg);
    const QList<Node    >& selected()  const;
    const QList<DetInfo >& detectors() const;
    const QList<ProcInfo>& segments () const;
    const QList<BldInfo >& reporters() const;
    QWidget*               display ();
  public slots:
    void select();
    void check_ready();
  private:
    PartitionControl& _pcontrol;
    enum { MAX_NODES=32 };
    Node _control;
    Node _segments [MAX_NODES];
    Node _events   [MAX_NODES];
    Node _recorders[MAX_NODES];
    NodeGroup* _segbox;
    NodeGroup* _evtbox;
    NodeGroup* _rptbox;
    QList<Node>    _selected;
    QList<DetInfo > _detinfo;
    QList<ProcInfo> _seginfo;
    QList<BldInfo > _rptinfo;
    QPushButton*   _acceptb;
  };
};

#endif
