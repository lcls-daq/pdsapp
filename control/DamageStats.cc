#include "DamageStats.hh"
#include "QCounter.hh"

#include "pdsapp/control/PartitionSelect.hh"
#include "pdsapp/tools/SummaryDg.hh"
#include "pds/management/PartitionControl.hh"
#include "pdsdata/psddl/alias.ddl.h"
#include "pdsdata/xtc/DetInfo.hh"
#include "pdsdata/xtc/BldInfo.hh"

#include <QtCore/QString>
#include <QtGui/QLabel>
#include <QtGui/QGridLayout>

#include <stdio.h>

using namespace Pds;

static inline bool matches(const Src& a, const Src& b)
{
  //  return a.level()==b.level() && a.phy()==b.phy();
  return a==b;
}

DamageStats::DamageStats(PartitionSelect& partition,
                         const PartitionControl& pcontrol) :
  QWidget(0),
  _partition(partition),
  _pcontrol (pcontrol )
{
  setWindowTitle("Damage Stats");

  QGridLayout* l = new QGridLayout(this);
  unsigned bldProcess=0;
  int row=0;
  l->addWidget(new QLabel("Source"), row, 0, Qt::AlignRight);
  l->addWidget(new QLabel("Events"), row, 1, Qt::AlignRight);
  row++;
  for(int i=0; i<_partition.detectors().size(); i++) {
    const DetInfo&  det  = _partition.detectors().at(i);
    const ProcInfo& proc = _partition.segments ().at(i);
    if (det.detector() != DetInfo::BldEb) {
      const char* alias = _pcontrol.lookup_src_alias(det);
      if (alias) {
        QLabel* label = new QLabel(alias);
        label->setToolTip(DetInfo::name(det));
        l->addWidget(label,row,0,Qt::AlignRight);
      }
      else {
        l->addWidget(new QLabel(DetInfo::name(det)),row,0,Qt::AlignRight);
      }
      QCounter* cnt = new QCounter;
      l->addWidget(cnt->widget(),row,1,Qt::AlignRight);
      _counts   << cnt;
      _segments << proc;
      row++;
    }
    else
      bldProcess = proc.processId();
  }
  if (bldProcess) {
    for(int i=0; i<_partition.reporters().size(); i++) {
      const BldInfo& info = _partition.reporters().at(i);
      l->addWidget(new QLabel(BldInfo::name(info)),row,0,Qt::AlignRight);
      QCounter* cnt = new QCounter;
      l->addWidget(cnt->widget(),row,1,Qt::AlignRight);
      _counts   << cnt;
      _segments << info;
      row++;
    }
    //
    //  Special EBeam BPM damage counter
    //
    l->addWidget(new QLabel("EBeam Low Curr"),row,0,Qt::AlignRight);
    QCounter* cnt = new QCounter;
    l->addWidget(cnt->widget(),row,1,Qt::AlignRight);
    _counts   << cnt;
    _segments << EBeamBPM;
    row++;
  }
  setLayout(l);

  update_stats();
  /*
  printf("DamageStats list:\n");
  for(int i=0; i<_segments.size(); i++)
    printf("  [%08x.%08x]\n",_segments[i].log(),_segments[i].phy());
  */
}

DamageStats::~DamageStats()
{
}

void DamageStats::increment(const SummaryDg& dg)
{
  static int _ndbgPrints=32;

  const QList<Src>& segm = _segments;
  for(unsigned j=0; j<dg.nSources(); j++) {
    const Src& info = dg.source(j);
    int i=0;
    /*
    printf("incr [%08x.%08x]\n", info.log(),info.phy());
    */
    while( i<segm.size() ) {
      if (matches(segm.at(i),info)) {
	_counts.at(i)->increment();
	break;
      }
      i++;
    }
    if (i==segm.size() && _ndbgPrints) {
      printf("DmgStats no match for proc [%08x.%08x]\n",
             info.log(), info.phy());
      _ndbgPrints--;
    }
  }
}

void DamageStats::update_stats()
{
  foreach(QCounter* cnt, _counts) {
    cnt->update_count();
  }
}

void DamageStats::dump() const
{
  unsigned bldProcess=0;
  int row=0;
  for(int i=0; i<_partition.detectors().size(); i++) {
    const DetInfo&  det  = _partition.detectors().at(i);
    const ProcInfo& proc = _partition.segments ().at(i);
    if (det.detector() != DetInfo::BldEb) {
      printf("%20.20s.%08x: %lld\n", DetInfo::name(det), det.phy(), _counts.at(row)->value());
      row++;
    }
    else
      bldProcess = proc.processId();
  }
  if (bldProcess) {
    for(int i=0; i<_partition.reporters().size(); i++) {
      const BldInfo& info = _partition.reporters().at(i);
      printf("%20.20s.%08x: %lld\n", BldInfo::name(info), info.phy(), _counts.at(row)->value());
      row++;
    }
    //
    //  Special EBeam BPM damage counter
    //
    printf("%20.20s.%08x: %lld\n", "EBeam Low Curr", EBeamBPM.phy(), _counts.at(row)->value());
    row++;
  }
}
