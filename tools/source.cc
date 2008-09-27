#include "pds/collection/CollectionManager.hh"
#include "pds/collection/Message.hh"

#include <time.h> // Required for timespec struct and nanosleep()

using namespace Pds;

class SourceLevel: public CollectionManager {
public:
  SourceLevel();
  virtual ~SourceLevel();

private:
  // Implements CollectionManager
  virtual void message(const Node& hdr, const Message& msg);
  virtual void connected(const Node& hdr, const Message& msg);
  virtual void timedout();
  virtual void disconnected();
};


static const unsigned MaxPayload = sizeof(Message);

SourceLevel::SourceLevel() :
  CollectionManager(MaxPayload, NULL)
{}

SourceLevel::~SourceLevel() {}

void SourceLevel::message(const Node& hdr, const Message& msg) {}

void SourceLevel::connected(const Node& hdr, const Message& msg) {}

void SourceLevel::timedout() {}

void SourceLevel::disconnected() {}


int main()
{
  SourceLevel source;

  source.connect();

  fprintf(stdout, "Commands: EOF=quit\n");
  while (true) {
    const int maxlen=128;
    char line[maxlen];
    char* result = fgets(line, maxlen, stdin);
    if (!result) {
      fprintf(stdout, "\nExiting\n");
      break;
    }  
  } 

  return 0;
}
