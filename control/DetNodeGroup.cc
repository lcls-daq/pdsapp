#include "pdsapp/control/DetNodeGroup.hh"

#include "pdsdata/xtc/DetInfo.hh"

#include <QtGui/QAbstractButton>
#include <QtGui/QBoxLayout>
#include <QtGui/QButtonGroup>
#include <QtGui/QGroupBox>

using namespace Pds;

DetNodeGroup::DetNodeGroup(const QString& label, 
			   QWidget*       parent, 
			   unsigned       platform, 
			   bool           useReadoutGroup, 
			   bool           useTransient) :
  NodeGroup(label, parent, platform,
	    useReadoutGroup, useTransient)
{
}

DetNodeGroup::~DetNodeGroup() 
{
}

QList<DetInfo> DetNodeGroup::detectors() 
{
  QList<DetInfo> dets;
  QList<QAbstractButton*> buttons = _buttons->buttons();
  foreach(QAbstractButton* b, buttons) {
    if (b->isChecked()) {
      int id = _buttons->id(b);
      if (_nodes[id].det().device()==DetInfo::Evr)
        dets.push_front(_nodes[id].det());
      else
        foreach(const NodeSelect& node, expanded(id))
          dets.push_back (node.det());
    }
  }
  return dets;
}

