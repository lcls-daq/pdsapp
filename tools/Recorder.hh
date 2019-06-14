#ifndef PDS_RECORDER
#define PDS_RECORDER

#include "pds/offlineclient/OfflineClient.hh"
#include "pds/utility/Appliance.hh"
#include "pdsdata/xtc/Src.hh"
#include "pdsdata/index/IndexList.hh"

#include <stdint.h>

namespace Pds {

class GenericPool;

class Recorder : public Appliance {
public:
   Recorder(const char* fname, unsigned int sliceID, uint64_t chunkSize, bool delay_xfer, OfflineClient *offline, const char* expname, unsigned uSizeThreshold);  
  ~Recorder() {}
  virtual Transition* transitions(Transition*);
  virtual InDatagram* occurrences(InDatagram* in);
  virtual InDatagram* events     (InDatagram* in);

private:
  int _openOutputFile(bool verbose);
  int _postDataFileError();
  int _writeOutputFile(const void *ptr, size_t size, size_t nmemb);
  int _writeSmallDataFile(const void *ptr, size_t size);
  int _flushOutputFile();
  int _flushSmallDataFile();
  int _closeOutputFile();
  int _renameOutputFile(bool verbose);
  int _renameOutputFile(int run, bool verbose);
  int _renameFile(char *oldName, char *newName, bool verbose);
  int _requestChunk();

  FILE* _f;
  FILE* _sdf;
  Pool* _pool;
  enum { SizeofPath=128 };
  char     _path[SizeofPath];
  enum { SizeofConfig=0x800000 };
  char     _config[SizeofConfig];
  char     _smlconfig[SizeofConfig];
  Src      _src;
  unsigned _node;
  unsigned int _sliceID;
  unsigned _beginrunerr;
  bool     _path_error;
  bool     _write_error;
  bool     _sdf_write_error;
  bool     _chunk_requested;
  enum { SizeofName=512 };
  char     _fname[SizeofName];
  char     _fnamerunning[SizeofName];
  char     _sdfname[SizeofName];
  char     _sdfnamerunning[SizeofName];
  unsigned int _chunk;
  uint64_t _chunkSize;
  bool     _delay_xfer;
  char     _expname[SizeofPath];
  int      _run;
  GenericPool* _occPool;
  Index::IndexList _indexList;
  char     _indexfname[SizeofName];
  char     _host_name[SizeofName];
  OfflineClient *_offlineclient;
  bool     _open_data_file_error;
  unsigned _uSizeThreshold;
};

}
#endif
