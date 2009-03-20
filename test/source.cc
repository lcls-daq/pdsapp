#include "pds/management/SourceLevel.hh"

#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

using namespace Pds;

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
    fprintf(stdout, "Commands: {s=Show partitions, EOF=quit}\n");
    while (true) {
      const int maxlen=128;
      char line[maxlen];
      char* result = fgets(line, maxlen, stdin);
      if (!result) {
        fprintf(stdout, "\nExiting\n");
        break;
      }  
      else if (result[0]=='s') {
	source.dump();
      }
    }
  } else {
    printf("*** Unable to connect: no interface 0x%x found\n", interface);
  }
  source.cancel();

  return 0;
}
