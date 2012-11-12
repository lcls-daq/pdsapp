// $Id$
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>

#include "pds/offlineclient/OfflineClient.hh"

#define MAXLINE 255
#define DEFAULT_STATION 0u

#define DEFAULT_RCPATH  "/reg/g/pcds/dist/pds/misc/.offlinereader"

#include <string>
#include <vector>
#include <utility>

using namespace Pds;

typedef std::vector<PartitionDescriptor> descList_t;

void usage(char* progname) {
  fprintf(stderr,"Usage: %s [OPTION]... <instrument name>[:<station #>]...\n", progname);
  fprintf(stderr,"  -h         help\n");
  fprintf(stderr,"  -v         verbose (may be repeated)\n");
  fprintf(stderr,"  -p <path>  database login path (default=%s)\n", DEFAULT_RCPATH);
}

static bool fileExists(const char *path) {
  struct stat buf;
  bool rv = false;

  if (path && (stat(path, &buf) == 0)) {
    if (S_ISREG(buf.st_mode)) {
      rv = true;
    }
  }
  return (rv);
}

//
// instrumentPrint -
//
// RETURNS: 1 on error, otherwise 0.
//
int instrumentPrint(const char *offlinerc, PartitionDescriptor& desc, unsigned int verbose, const char *argv0) {
  int rv = 1;   // return error by default

  OfflineClient* offlineclient = new OfflineClient(offlinerc, desc, (verbose > 1));

  if (offlineclient == NULL) {
    fprintf(stderr,"%s: failed to instantiate OfflineClient\n", argv0);
  } else if (offlineclient->GetExperimentName() == NULL) {
    fprintf(stderr,"%s: failed to find current experiment name\n", argv0);
  } else {
    rv = 0;         // return OK
    if (verbose) {
      printf("Instrument name: %s\n", offlineclient->GetInstrumentName());
      printf("Station number: %u\n", offlineclient->GetStationNumber());
      printf("Experiment name: %s\n", offlineclient->GetExperimentName());
      printf("Experiment number: %u\n", offlineclient->GetExperimentNumber());
    } else {
      printf("%s:%u %s %u\n",  offlineclient->GetInstrumentName(),
                               offlineclient->GetStationNumber(),
                               offlineclient->GetExperimentName(),
                               offlineclient->GetExperimentNumber());
    }
  }

  if (offlineclient != NULL) {
    delete offlineclient;
  }

  return (rv);
}

int main(int argc, char* argv[]) {
  int ii;
  int parseErr = 0;
  char *rcpath = DEFAULT_RCPATH;
  unsigned int verbose = 0;
  descList_t descList;
  descList_t::iterator it;

  while ((ii = getopt(argc, argv, "hp:v")) != -1) {
    switch (ii) {
    case 'h':
      usage(argv[0]);
      exit(0);
    case 'p':
      rcpath = optarg;
      break;
    case 'v':
      ++verbose;
      break;
    default:
      parseErr++;
    }
  }

  if (optind > (argc - 1)) {
    // must specify at least one instrument
    parseErr++;
  } else {
    for (ii = optind; ii < argc; ii++) {
      PartitionDescriptor pd(argv[ii]);
      if (!pd.valid()) {
        parseErr++;
        break;
      } else {
        descList.push_back(pd);
      }
    }
  }

  if (parseErr > 0) {
    usage(argv[0]);
    exit(1);
  }

  if (!fileExists(rcpath)) {
    fprintf(stderr, "%s: %s: No such file\n", argv[0], rcpath);
    exit(1);
  }

  int rv = 0;

  for (it = descList.begin(); it != descList.end(); it++) {
    if (instrumentPrint(rcpath, *it, verbose, argv[0]) != 0) {
      rv = 1;
      break;
    }
  }

  return (rv);
}
