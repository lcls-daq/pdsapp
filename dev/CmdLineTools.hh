#ifndef PdsApp_CmdLineTools_hh
#define PdsApp_CmdLineTools_hh

namespace Pds {
  class DetInfo;
  class CmdLineTools {
  public:
    static bool parseDetInfo(const char* args, DetInfo& info);
  };
};

#endif
