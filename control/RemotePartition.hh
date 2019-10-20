#ifndef Pds_RemotePartition_hh
#define Pds_RemotePartition_hh

static const unsigned ModifyPartition = 0x80000000;
static const unsigned RecordSetMask   = 0x40000000;
static const unsigned RecordValMask   = 0x20000000;
static const unsigned DbKeyMask       = 0x0fffffff;

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
    enum { NameSize=64 };
    enum { ReadoutMask = 1,
	   RecordMask  = 2 };
    char     _name[NameSize];
    uint32_t _phy;
    uint32_t _options;
  };

  class RemotePartition {
  public:
    RemotePartition() : _nnodes(0), _options(0), _unbiased_f(0), _l3path() {}
  public:
    bool l3tag () const { return _options&L3Tag; }
    bool l3veto() const { return _options&L3Veto; }
    float l3uf () const { return _unbiased_f; }
    const char* l3path() const { return _l3path; }
    unsigned nodes() const { return _nnodes; }
    RemoteNode* node(unsigned j) { return &_nodes[j]; }
    const RemoteNode* node(unsigned j) const { return &_nodes[j]; }
  public:
    void set_l3t (const char* path, bool veto=false, float uf=0) 
    { 
      strncpy(_l3path,path,MaxName); 
      _options |= L3Tag | (veto ? L3Veto:0);
      _unbiased_f = uf;
    }
    void clear_l3t() { _options &= ~(L3Veto|L3Tag); }
    void add_node(const RemoteNode& n) { _nodes[_nnodes++]=n; }
  private:
    enum { MaxNodes=64 };
    enum { MaxName=128 };
    enum { L3Tag=1, L3Veto=2 };
    uint32_t   _nnodes;
    uint32_t   _options;
    float      _unbiased_f;
    char       _l3path[MaxName];
    RemoteNode _nodes[MaxNodes];
  };
};

#endif
