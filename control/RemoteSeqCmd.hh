#ifndef Pds_RemoteSeqCmd_hh
#define Pds_RemoteSeqCmd_hh

#include <stdint.h>

namespace Pds
{
  struct RemoteSeqCmd
  {
    enum { CMD_SIGNATURE = 0x0000FFFF };
    enum { VERSION       = 0x1        };
    
  public:
    enum CmdType
    {
      CMD_GET_CUR_EVENT_NUM = 0x1
    };
  
    uint32_t  u32Signature;
    uint32_t  u32Version;
    uint32_t  u32Type;
    uint32_t  u32Op1;
    uint32_t  u32Op2;
    
    RemoteSeqCmd() {}
    
    RemoteSeqCmd(uint32_t u32Type1) :
      u32Signature(CMD_SIGNATURE),
      u32Version  (VERSION),
      u32Type     (u32Type1)
    {
    }

    bool IsValid()
    {
      return u32Signature == CMD_SIGNATURE && u32Version == VERSION;
    }    
  };  
}

#endif
