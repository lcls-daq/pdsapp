#ifndef Pds_BldNodeGroup_hh
#define Pds_BldNodeGroup_hh

#include "pdsapp/control/NodeGroup.hh"

namespace Pds {

  class BldNodeGroup : public NodeGroup {
  public:
    BldNodeGroup(const QString& label, 
		 QWidget*       parent, 
		 unsigned       platform, 
		 int            iUseReadoutGroup = 0, 
		 bool           useTransient=false);
    ~BldNodeGroup();
  protected:
    virtual int    order(const NodeSelect&, const QString&);
  public:
    QList<BldInfo> reporters();
    QList<BldInfo> transients();
  private:
    QList<int>     _order;
  };
}; // namespace Pds

#endif
