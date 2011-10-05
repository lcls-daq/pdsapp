#ifndef PDS_EVENTOPTIONS_HH
#define PDS_EVENTOPTIONS_HH

#include <stdint.h>

namespace Pds {
class EventOptions {
public:
  EventOptions();
  EventOptions(int argc, char** argv);

  int validate(const char*) const;

public:  
  unsigned platform;
  unsigned sliceID;
  unsigned buffersize;
  const char* arpsuidprocess;
  const char* outfile;

  enum Mode {Counter, Decoder, Display};
  Mode mode;
  uint64_t chunkSize;
  bool     delayXfer;
}; 
}

#endif
