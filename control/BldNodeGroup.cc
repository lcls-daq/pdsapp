#include "pdsapp/control/BldNodeGroup.hh"

#include "pdsdata/xtc/BldInfo.hh"

#include <QtGui/QAbstractButton>
#include <QtGui/QBoxLayout>
#include <QtGui/QButtonGroup>
#include <QtGui/QGroupBox>

using namespace Pds;

static QList<int> _bldOrder = 
  QList<int>() << BldInfo::EBeam
         << BldInfo::EOrbits
         << BldInfo::PhaseCavity
         << BldInfo::FEEGasDetEnergy
         << BldInfo::Nh2Sb1Ipm01
         << BldInfo::Nh2Sb1Ipm02
         << BldInfo::HxxUm6Imb01
         << BldInfo::HxxUm6Imb02                           
         << BldInfo::HxxDg1Cam
         << BldInfo::HfxDg2Imb01
         << BldInfo::HfxDg2Imb02
         << BldInfo::HfxDg2Cam
         << BldInfo::HfxMonImb01
         << BldInfo::HfxMonImb02
         << BldInfo::HfxMonImb03
         << BldInfo::HfxMonCam
         << BldInfo::HfxDg3Imb01
         << BldInfo::HfxDg3Imb02
         << BldInfo::HfxDg3Cam
         << BldInfo::XcsDg3Imb03
         << BldInfo::XcsDg3Imb04
         << BldInfo::XcsDg3Cam
         << BldInfo::MecLasEm01
         << BldInfo::MecTctrPip01
         << BldInfo::MecTcTrDio01
         << BldInfo::MecXt2Ipm02
         << BldInfo::MecXt2Ipm03
         << BldInfo::MecHxmIpm01
         << BldInfo::GMD
         << BldInfo::CxiDg1Imb01
         << BldInfo::CxiDg2Imb01
         << BldInfo::CxiDg2Imb02
         << BldInfo::CxiDg3Imb01
         << BldInfo::CxiDg1Pim
         << BldInfo::CxiDg2Pim
         << BldInfo::CxiDg3Spec
         << BldInfo::CxiDg3Pim
         << BldInfo::NumberOf;

BldNodeGroup::BldNodeGroup(const QString& label, 
			   QWidget*       parent, 
			   unsigned       platform, 
			   int            iUseReadoutGroup, 
			   bool           useTransient) :
  NodeGroup(label, parent, platform,
	    iUseReadoutGroup, useTransient)
{
}

BldNodeGroup::~BldNodeGroup() 
{
}

QList<BldInfo> BldNodeGroup::reporters() 
{
  QList<BldInfo> dets;
  QList<QAbstractButton*> buttons = _buttons->buttons();
  foreach(QAbstractButton* b, buttons) {
    if (b->isChecked()) {
      int id = _buttons->id(b);
      dets.push_back(_nodes[id].bld());
    }
  }
  return dets;
}

QList<BldInfo> BldNodeGroup::transients() 
{
  QList<BldInfo> dets;
  QList<QAbstractButton*> buttons = _buttons->buttons();
  foreach(QAbstractButton* b, buttons) {
    if (b->isChecked()) {
      int id = _buttons->id(b);
      if (_nodes[id].node().transient())
	dets.push_back(_nodes[id].bld());
    }
  }
  return dets;
}

int BldNodeGroup::order(const NodeSelect& node, const QString&)
{
  QBoxLayout* l = static_cast<QBoxLayout*>(_group->layout());
  int order = _bldOrder.indexOf(node.src().phy());
  int index;
  for(index = 0; index < l->count(); index++) {
    if (order < _order[index])
      break;
  }
  _order.insert(index,order);

  return index;
}
