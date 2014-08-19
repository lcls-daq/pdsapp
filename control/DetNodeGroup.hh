#ifndef Pds_DetNodeGroup_hh
#define Pds_DetNodeGroup_hh

#include "pdsapp/control/NodeGroup.hh"

namespace Pds {

  class DetNodeGroup : public NodeGroup {
  public:
    DetNodeGroup(const QString& label, 
		 QWidget*       parent, 
		 unsigned       platform, 
		 int            iUseReadoutGroup=0, 
		 bool           useTransient=false);
    ~DetNodeGroup();
  public:
    QList<DetInfo> detectors();
  };
}; // namespace Pds

#endif
