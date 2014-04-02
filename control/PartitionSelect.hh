#ifndef Pds_PartitionSelect_hh
#define Pds_PartitionSelect_hh

#include <set>
#include <QtGui/QGroupBox>
#include <QtCore/QList>

#include "pds/collection/Node.hh"
#include "pdsdata/xtc/BldInfo.hh"
#include "pdsdata/xtc/DetInfo.hh"
#include "pdsdata/xtc/ProcInfo.hh"

class QPushButton;

namespace Pds {

  class PartitionControl;
  class IocControl;

  class PartitionSelect : public QGroupBox {
    Q_OBJECT
  public:
    PartitionSelect(QWidget*          parent,
                    PartitionControl& control,
                    IocControl&       icontrol,
                    const char*       pt_name,
                    const char*       db_name,
                    unsigned          options);
    ~PartitionSelect();
  public:
    const QList<DetInfo >& detectors() const;
    const std::set<std::string>& deviceNames() const;
    const QList<ProcInfo>& segments () const;
    const QList<BldInfo >& reporters() const;
  public slots:
    void select_dialog();
    void display      ();
    void change_state(QString);
    void autorun      ();
  private:
    bool _validate(uint64_t);
    bool _checkReadGroupEnable();
  private:
    PartitionControl&  _pcontrol;
    IocControl&        _icontrol;
    const char*        _pt_name;
    char               _db_path[128];
    QWidget*           _display;
    unsigned           _options;
    enum { MAX_NODES=64 };
    unsigned _nnodes;
    Node _nodes[MAX_NODES];
    QList<DetInfo > _detectors;
    std::set<std::string> _deviceNames;
    QList<ProcInfo> _segments;
    QList<BldInfo > _reporters;
    QPushButton*    _selectb;
    bool            _autorun;
  };
};

#endif
