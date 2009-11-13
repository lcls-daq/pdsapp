#ifndef Pds_PartitionSelect_hh
#define Pds_PartitionSelect_hh

#include <QtGui/QGroupBox>
#include <QtCore/QList>

#include "pds/collection/Node.hh"
#include "pdsdata/xtc/DetInfo.hh"

namespace Pds {

  class PartitionControl;

  class PartitionSelect : public QGroupBox {
    Q_OBJECT
  public:
    PartitionSelect(QWidget*          parent,
		    PartitionControl& control,
		    const char*       pt_name,
		    const char*       db_name);
    ~PartitionSelect();
  public:
    const QList<DetInfo>& detectors() const;
  public slots:
    void select_dialog();
    void display      ();
  private:
    PartitionControl&  _pcontrol;
    const char*        _pt_name;
    char               _db_path[128];
    QWidget*           _display;
    enum { MAX_NODES=64 };
    unsigned _nnodes;
    Node _nodes[MAX_NODES];
    QList<DetInfo> _detectors;
  };
};

#endif
