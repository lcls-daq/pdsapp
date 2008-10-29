#include "pds/collection/CollectionSource.hh"
#include "pds/collection/Message.hh"

#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

class SourcePing : public Pds::Message {
public:
  SourcePing(unsigned pleasereply) :
    Pds::Message(Pds::Message::Ping, sizeof(SourcePing)),
    _reply(pleasereply)
  {}

  unsigned reply() const {return _reply;}

private:
  unsigned _reply;
};

static const unsigned MaxPayload = sizeof(SourcePing);

class SourceLevel: public Pds::CollectionSource {
public:
  SourceLevel() : Pds::CollectionSource(MaxPayload, NULL) {}
  virtual ~SourceLevel() {}

private:
  virtual void message(const Pds::Node& hdr, const Pds::Message& msg)
  {
    if (msg.type() == Pds::Message::Ping) {
      if (hdr.level() != Pds::Level::Source) {
        Pds::Message reply(msg.type());
        Pds::Ins dst = msg.reply_to();
        ucast(reply, dst);
      } else {
        const SourcePing& sp = reinterpret_cast<const SourcePing&>(msg);
        if (!(hdr == header())) {
          if (sp.reply()) {
            SourcePing reply(0);
            Pds::Ins dst = msg.reply_to();
            ucast(reply, dst);
          }
          in_addr ip; ip.s_addr = htonl(hdr.ip());
          printf("*** warning: another source level running on %s pid %d\n",
                 inet_ntoa(ip), hdr.pid());
        }
      }
    }
  }
};

int main(int argc, char** argv)
{
  if (argc != 2) {
    printf("usage: %s <interface>\n", argv[0]);
    return 0;
  }

  unsigned interface = 0;
  in_addr inp;
  if (inet_aton(argv[1], &inp)) {
    interface = ntohl(inp.s_addr);
  }

  if (!interface) {
    printf("Invalid <interface> argument %s\n", argv[1]);
    return 0;    
  }

  SourceLevel source;
  source.start();
  bool connected = source.connect(interface);
  if (connected) {
    SourcePing sp(1);
    source.mcast(sp);
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
  } else {
    printf("*** Unable to connect: no interface 0x%x found\n", interface);
  }
  source.cancel();

  return 0;
}
