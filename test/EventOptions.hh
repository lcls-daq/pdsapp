#ifndef PDS_EVENTOPTIONS_HH
#define PDS_EVENTOPTIONS_HH

namespace Pds {
class EventOptions {
public:
  EventOptions(int argc, char** argv);

  int validate(const char*) const;

public:  
  unsigned partition;
  unsigned buffersize;
  int id;
  const char* arpsuidprocess;

  enum Mode {Counter, Decoder, Display};
  Mode mode;
}; 
}

#endif
