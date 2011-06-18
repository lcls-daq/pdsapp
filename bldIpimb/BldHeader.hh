#ifndef Pds_BldHeader_hh
#define Pds_BldHeader_hh

#define BLD_HEADER_SIZE  60
#define EXTENT_OFFSET    40

namespace Pds {

#pragma pack(4)	
class BldHeader {
  public:
    BldHeader() {}  
    BldHeader(unsigned source2, unsigned contains, unsigned version, unsigned payloadSize) {
      _sec   = 0;  _nSec   = 0; _mbz_1  = 0;
      _fidId = 0;  _mbz_2  = 0; _damage_1 = 0; 	_damage_2 = 0;  
      _source1_1  = 0x06000000; _source1_2  = 0x06000000;    //Pds::Level::Reporter according to "pdsdata/xtc/Level.hh" 
      _source2_1  = source2;    _source2_2  = source2;
      _contains_1 = (version << 16) | contains;
      _contains_2 = _contains_1; 
      _extent_1   = BLD_HEADER_SIZE - EXTENT_OFFSET  + payloadSize;
      _extent_2   = _extent_1;
 
    }  
    ~BldHeader() {};
    void timeStamp(unsigned sec, unsigned nSec, unsigned fidId) {
      _sec = sec;
      _nSec = nSec;
      _fidId = fidId;
    }
    void setDamage(unsigned damage) { _damage_1 = damage; _damage_2 = damage; }

  private:
    uint32_t  _sec;
    uint32_t  _nSec;
    uint32_t  _mbz_1;
    uint32_t  _fidId;
    uint32_t  _mbz_2;
    uint32_t  _damage_1;
    uint32_t  _source1_1;
    uint32_t  _source2_1;
    uint32_t  _contains_1;
    uint32_t  _extent_1;
    uint32_t  _damage_2;
    uint32_t  _source1_2;
    uint32_t  _source2_2;
    uint32_t  _contains_2;
    uint32_t  _extent_2;
  };
#pragma pack()	  
};

#endif  

