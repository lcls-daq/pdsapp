#include "pds/service/CmdLineTools.hh"
#include "pds/jungfrau/Driver.hh"
#include "pds/jungfrau/Builder.hh"

#include <getopt.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>

static Pds::Jungfrau::ZmqDetector* det = NULL;

using namespace Pds;

static void shutdown(int signal)
{
  if (det) {
    det->abort();
  } else {
    exit(signal);
  }
}

static void jungfrauPartUsage(const char* p)
{
  printf("Usage: %s -H <host> -P <port> [OPTIONS]\n"
         "\n"
         "Options:\n"
         "    -H|--host     <host>                    set the zmq host ip\n"
         "    -P|--port     <port>                    set the zmq port number\n"
         "    -m|--mask     <mask>                    set the module id mask\n"
         "    -h|--help                               print this message and exit\n", p);
}

int main(int argc, char** argv) {

  const char*   strOptions    = ":hP:H:m:";
  const struct option loOptions[]   =
    {
       {"help",        0, 0, 'h'},
       {"host",        1, 0, 'H'},
       {"port",        1, 0, 'P'},
       {"mask",        1, 0, 'm'},
       {0,             0, 0,  0 }
    };

  unsigned mask  = 0;
  unsigned port  = 0;
  bool lUsage = false;
  char* hostname = (char *)NULL;
  std::vector<unsigned> module_ids;

  int optionIndex  = 0;
  while ( int opt = getopt_long(argc, argv, strOptions, loOptions, &optionIndex ) ) {
    if ( opt == -1 ) break;

    switch(opt) {
      case 'h':               /* Print usage */
        jungfrauPartUsage(argv[0]);
        return 0;
      case 'H':
        hostname = optarg;
        break;
      case 'P':
        if (!CmdLineTools::parseUInt(optarg,port)) {
          printf("%s: option `-P' parsing error\n", argv[0]);
          lUsage = true;
        }
        break;
      case 'm':
        if (!CmdLineTools::parseUInt(optarg,mask)) {
          printf("%s: option `-m' parsing error\n", argv[0]);
          lUsage = true;
        }
        break;
      case '?':
        if (optopt)
          printf("%s: Unknown option: %c\n", argv[0], optopt);
        else
          printf("%s: Unknown option: %s\n", argv[0], argv[optind-1]);
        lUsage = true;
        break;
      case ':':
        printf("%s: Missing argument for %c\n", argv[0], optopt);
      default:
        lUsage = true;
        break;
    }
  }

  // set the module list based on the mask
  for (unsigned i=0; i<JungfrauConfigType::MaxModulesPerDetector; i++) {
    if (mask & (1<<i))
      module_ids.push_back(i);
  }

  if(module_ids.empty()) {
    printf("%s: at least one module is required\n", argv[0]);
    lUsage = true;
  }

  if (!hostname) {
    printf("%s: hostname is required\n", argv[0]);
    lUsage = true;
  }

  if (!port) {
    printf("%s: port is required\n", argv[0]);
    lUsage = true;
  }

  if (optind < argc) {
    printf("%s: invalid argument -- %s\n", argv[0], argv[optind]);
    lUsage = true;
  }

  if (lUsage) {
    jungfrauPartUsage(argv[0]);
    return 1;
  }

  /*
   * Register singal handler
   */
  struct sigaction sigActionSettings;
  sigemptyset(&sigActionSettings.sa_mask);
  sigActionSettings.sa_handler = shutdown;
  sigActionSettings.sa_flags   = SA_RESTART;

  if (sigaction(SIGINT, &sigActionSettings, 0) != 0 )
    printf("Cannot register signal handler for SIGINT\n");
  if (sigaction(SIGTERM, &sigActionSettings, 0) != 0 )
    printf("Cannot register signal handler for SIGTERM\n");

  det = new Jungfrau::ZmqDetector(hostname, port);
  for (std::vector<unsigned>::iterator it = module_ids.begin() ; it != module_ids.end(); ++it) {
    det->add_module(*it);
  }

  det->run();

  delete det;

  return 0;
}
