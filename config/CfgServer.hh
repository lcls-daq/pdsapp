#ifndef Pds_CfgServer_hh
#define Pds_CfgServer_hh

#include "pds/service/GenericPool.hh"

namespace Pds {

  class CfgRequest;

  class CfgServer {
  public:
    CfgServer(unsigned platform,
	      unsigned max_size,
	      unsigned nclients);
    virtual ~CfgServer();
  public:
    void run();
    virtual int fetch(void*, const CfgRequest&) = 0;
  protected:
    unsigned _platform;
  private:
    GenericPool _pool;
    unsigned    _max_size;
  };

}

#endif
