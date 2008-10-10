#ifndef PDS_EVENTOPTIONS_HH
#define PDS_EVENTOPTIONS_HH

namespace Pds {
class EventOptions {
public:
  EventOptions(int argc, char** argv);

  int validate(const char*) const;

public:  
  unsigned platform;
  unsigned buffersize;
  const char* arpsuidprocess;

  enum Mode {Counter, Decoder, Display};
  Mode mode;
}; 
}

#endif
