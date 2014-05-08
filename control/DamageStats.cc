#include "DamageStats.hh"
#include "QCounter.hh"

#include "pdsapp/control/PartitionSelect.hh"
#include "pds/xtc/SummaryDg.hh"
#include "pds/management/PartitionControl.hh"
#include "pds/ioc/IocControl.hh"
#include "pds/ioc/IocNode.hh"
#include "pdsdata/psddl/alias.ddl.h"
#include "pdsdata/xtc/DetInfo.hh"
#include "pdsdata/xtc/BldInfo.hh"

#include <QtCore/QString>
#include <QtGui/QLabel>
#include <QtGui/QGridLayout>

#include <stdio.h>

//#define DBUG

using namespace Pds;

static inline bool matches(const Src& a, const Src& b)
{
  //  return a.level()==b.level() && a.phy()==b.phy();
  return a==b;
}

DamageStats::DamageStats(PartitionSelect& partition,
                         const PartitionControl& pcontrol,
                         const IocControl&       icontrol) :
  QWidget(0),
  _partition(partition),
  _pcontrol (pcontrol ),
  _icontrol (icontrol )
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
  for(std::list<IocNode*>::const_iterator it=icontrol.selected().begin();
      it!=icontrol.selected().end(); it++) {
    const IocNode* node = *it;
    l->addWidget(new QLabel(node->alias() ? node->alias() :
                            DetInfo::name(static_cast<const DetInfo&>(node->src()))),
                 row, 0, Qt::AlignRight);
    QCounter* cnt = new QCounter;
    l->addWidget(cnt->widget(),row,1,Qt::AlignRight);
    _counts   << cnt;
    _segments << node->src();
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

void DamageStats::increment(const SummaryDg::Xtc& xtc)
{
  static int _ndbgPrints=32;

  const QList<Src>& segm = _segments;
  for(unsigned j=0; j<xtc.nSources(); j++) {
    const Src& info = xtc.source(j);
    int i=0;
#ifdef DBUG
    printf("incr [%08x.%08x]\n", info.log(),info.phy());
#endif
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
  unsigned niocs = _icontrol.selected().size();
  unsigned i=0;
  while(i<_counts.size()-niocs)
    _counts.at(i++)->update_count();

  for(std::list<IocNode*>::const_iterator it=_icontrol.selected().begin();
      it!=_icontrol.selected().end(); it++,i++) {
    const IocNode* node = *it;
    QCounter* cnt = _counts.at(i);
    cnt->insert(node->damaged(), node->events());
    cnt->update_frac ();
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
      printf("%32.32s.%08x: %lld\n", BldInfo::name(info), info.phy(), _counts.at(row)->value());
      row++;
    }
    //
    //  Special EBeam BPM damage counter
    //
    printf("%20.20s.%08x: %lld\n", "EBeam Low Curr", EBeamBPM.phy(), _counts.at(row)->value());
    row++;
  }
  for(std::list<IocNode*>::const_iterator it=_icontrol.selected().begin();
      it!=_icontrol.selected().end(); it++,row++) {
    const IocNode* node = *it;
    printf("%32.32s.%08x: %s\n", 
           node->alias() ? node->alias() : DetInfo::name(static_cast<const DetInfo&>(node->src())),
           node->src().phy(),
           qPrintable(_counts.at(row)->widget()->text()));
  }
}
