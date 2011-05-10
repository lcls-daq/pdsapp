#ifndef PDS_RECORDER
#define PDS_RECORDER

#include <stdint.h>

#include "pdsdata/xtc/Src.hh"
#include "pdsdata/index/IndexList.hh"
#include "pds/utility/Appliance.hh"

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
  int _postDataFileError();
  int _writeOutputFile(const void *ptr, size_t size, size_t nmemb);
  int _flushOutputFile();
  int _closeOutputFile();
  int _renameOutputFile(bool verbose);
  int _renameOutputFile(int run, bool verbose);
  int _renameFile(char *oldName, char *newName, bool verbose);
  int _requestChunk();

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
  bool     _write_error;
  bool     _chunk_requested;
  enum { SizeofName=512 };
  char     _fname[SizeofName];
  char     _fnamerunning[SizeofName];
  unsigned int _chunk;
  uint64_t _chunkSize;
  int      _experiment;
  int      _run;
  GenericPool* _occPool;
  Index::IndexList _indexList;
  char     _indexfname[SizeofName];
};

}
#endif
