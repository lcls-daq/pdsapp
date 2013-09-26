#ifndef Pds_DamageStats_hh
#define Pds_DamageStats_hh

#include "pdsdata/xtc/Src.hh"
#include <QtGui/QWidget>
#include <QtCore/QList>

namespace Pds {
  namespace Alias { class ConfigV1; }
  class InDatagramIterator;
  class PartitionSelect;
  class PartitionControl;
  class QCounter;
  namespace SummaryDg { class Xtc; }

  class DamageStats : public QWidget {
    Q_OBJECT
  public:
    DamageStats(PartitionSelect&,
                const PartitionControl&);
    ~DamageStats();
  public:
    void  increment(const SummaryDg::Xtc&);
    void  dump() const;
  public slots:
    void  update_stats();
  private:
    PartitionSelect& _partition;
    const PartitionControl& _pcontrol;
    QList<Src>       _segments;
    QList<QCounter*> _counts;
  };
};

#endif
