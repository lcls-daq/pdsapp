#ifndef PDS_RECORDER
#define PDS_RECORDER

#include "pds/utility/Appliance.hh"
#include "pdsdata/xtc/Src.hh"
#include <stdint.h>

namespace Pds {

class GenericPool;

class Recorder : public Appliance {
public:
  Recorder(const char* fname, unsigned int sliceID, uint64_t chunkSize);
  ~Recorder() {}
  virtual Transition* transitions(Transition*);
  virtual InDatagram* occurrences(InDatagram* in);
  virtual InDatagram* events     (InDatagram* in);

private:
  int _openOutputFile(bool verbose);
  int _renameOutputFile(bool verbose);
  int _renameOutputFile(int run, bool verbose);
  int _renameFile(char *oldName, char *newName, bool verbose);

  FILE* _f;
  Pool* _pool;
  enum { SizeofPath=128 };
  char     _path[SizeofPath];
  enum { SizeofConfig=0x800000 };
  char     _config[SizeofConfig];
  Src      _src;
  unsigned _node;
  unsigned int _sliceID;
  unsigned _beginrunerr;
  bool     _path_error;
  enum { SizeofName=512 };
  char     _fname[SizeofName];
  char     _fnamerunning[SizeofName];
  unsigned int _chunk;
  uint64_t _chunkSize;
  int      _experiment;
  int      _run;
  GenericPool* _occPool;
};

}
#endif
