#ifndef Pds_RemotePartition_hh
#define Pds_RemotePartition_hh

static const unsigned ModifyPartition = 0x40000;
static const unsigned RecordSetMask   = 0x20000;
static const unsigned RecordValMask   = 0x10000;
static const unsigned DbKeyMask       = 0x0ffff;

namespace Pds {
  class RemoteNode {
  public:
    RemoteNode() {}
    RemoteNode(const char* n, unsigned p) : 
      _phy(p), _options(ReadoutMask|RecordMask) 
    { strncpy(_name,n,NameSize); }
  public:
    const char* name   () const { return _name; }
    unsigned    phy    () const { return _phy; }
    bool        readout() const { return _options&ReadoutMask; }
    bool        record () const { return _options&RecordMask; }
  public:
    void        record (bool v) { 
      _options |= ReadoutMask;
      if (v) _options |=  RecordMask;
      else   _options &= ~RecordMask;
    }
    void        readout(bool v) {
      if (v) _options |=  ReadoutMask;
      else   _options &= ~ReadoutMask;
    }
  private:
    enum { NameSize=32 };
    enum { ReadoutMask = 1,
	   RecordMask  = 2 };
    char     _name[NameSize];
    uint32_t _phy;
    uint32_t _options;
  };

  class RemotePartition {
  public:
    RemotePartition() : _nnodes(0) {}
  public:
    unsigned nodes() const { return _nnodes; }
    RemoteNode* node(unsigned j) { return &_nodes[j]; }
    const RemoteNode* node(unsigned j) const { return &_nodes[j]; }
    void add_node(const RemoteNode& n) { _nodes[_nnodes++]=n; }
  private:
    enum { MaxNodes=64 };
    uint32_t   _nnodes;
    uint32_t   _reserved;
    RemoteNode _nodes[MaxNodes];
  };
};

#endif
