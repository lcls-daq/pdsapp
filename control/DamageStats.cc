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
  foreach(DetInfo info, _partition.detectors()) {
    if (info.detector() != DetInfo::NoDetector &&
	info.detector() != DetInfo::EpicsArch)
      _detectors << info;
  }

  QGridLayout* l = new QGridLayout(this);
  const QList<DetInfo>& detectors = _detectors;
  int row=0;
  foreach(DetInfo info, detectors) {
    QCounter* cnt = new QCounter;
    _counts << cnt;
    l->addWidget(new QLabel(QString("%1/%2")
			    .arg(DetInfo::name(info.detector()))
			    .arg(DetInfo::name(info.device())),this),
		 row,0,Qt::AlignRight);
    l->addWidget(cnt->widget(),row,1,Qt::AlignLeft);
    row++;
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
  foreach(QCounter* cnt, _counts) {
    cnt->increment();
  }
  const QList<DetInfo>& det = _detectors;
  DetInfo info;
  while(extent) {
    advance += iter->copy(&info, sizeof(info));
    extent  -= sizeof(info);
    for(int i=0; i<det.size(); i++) {
      if (det.at(i)==info)
	_counts.at(i)->decrement();
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
