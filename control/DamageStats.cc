#include "DamageStats.hh"
#include "QCounter.hh"

#include "pdsapp/control/PartitionSelect.hh"
#include "pds/xtc/InDatagramIterator.hh"
#include "pdsdata/xtc/DetInfo.hh"

#include <QtCore/QString>
#include <QtGui/QLabel>
#include <QtGui/QGridLayout>

using namespace Pds;

DamageStats::DamageStats(PartitionSelect& partition) :
  QWidget(0),
  _partition(partition)
{
  setWindowTitle("Damage Stats");

  QGridLayout* l = new QGridLayout(this);
  int row=0;
  for(int i=0; i<_partition.detectors().size(); i++) {
    const DetInfo&  det  = _partition.detectors().at(i);
    const ProcInfo& proc = _partition.segments ().at(i);
    if (det.detector() != DetInfo::NoDetector) {
      QCounter* cnt = new QCounter;
      _counts   << cnt;
      _segments << proc;
      l->addWidget(new QLabel(DetInfo::name(det)),row,0,Qt::AlignRight);
      l->addWidget(cnt->widget(),row,1,Qt::AlignLeft);
      row++;
    }
  }
  setLayout(l);

  update_stats();
}

DamageStats::~DamageStats()
{
}

int DamageStats::increment(InDatagramIterator* iter, int extent)
{
  int advance=0;

  const QList<ProcInfo>& segm = _segments;
  ProcInfo info(Level::Control,0,0);
  while(extent) {
    advance += iter->copy(&info, sizeof(info));
    extent  -= sizeof(info);
    for(int i=0; i<segm.size(); i++) {
      if (segm.at(i)==info)
	_counts.at(i)->increment();
    }
  }
  return advance;
}

void DamageStats::update_stats()
{
  foreach(QCounter* cnt, _counts) {
    cnt->update_count();
  }
}
