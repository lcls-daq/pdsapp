#ifndef Pds_CfgFileDb_hh
#define Pds_CfgFileDb_hh

namespace Pds {

  class CfgRequest;

  class CfgFileDb {
  public:
    CfgFileDb(const char* directory,
	      unsigned    platform);
    ~CfgFileDb();
  public:
    int open (const CfgRequest&, int mode);
    int close(int);
  private:
    char* _path;
    int   _offset;
  };

}

#endif
