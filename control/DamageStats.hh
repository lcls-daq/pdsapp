#ifndef Pds_DamageStats_hh
#define Pds_DamageStats_hh

#include "pdsdata/xtc/ProcInfo.hh"
#include <QtGui/QWidget>
#include <QtCore/QList>

namespace Pds {
  class InDatagramIterator;
  class PartitionSelect;
  class QCounter;

  class DamageStats : public QWidget {
    Q_OBJECT
  public:
    DamageStats(PartitionSelect&);
    ~DamageStats();
  public:
    int   increment(InDatagramIterator*,int);
    void  dump() const;
  public slots:
    void  update_stats();
  private:
    PartitionSelect& _partition;
    QList<ProcInfo>  _segments;
    QList<QCounter*> _counts;
  };
};

#endif
