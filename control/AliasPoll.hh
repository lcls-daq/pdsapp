#ifndef Pds_AliasPoll_hh
#define Pds_AliasPoll_hh

#include "pds/management/PlatformCallback.hh"
#include "pds/config/AliasFactory.hh"

namespace Pds {
  class PartitionControl;
  class AliasPoll : public PlatformCallback {
  public:
    AliasPoll(PartitionControl&);
    ~AliasPoll();
  public:
    void        available   (const Node& hdr, const PingReply& msg);
    void        aliasCollect(const Node& hdr, const AliasReply& msg);
    const AliasFactory&    aliases() const;
  private:
    PartitionControl& _control;
    AliasFactory      _aliases;
  };
};

#endif
