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
#define DEFAULT_STATION 0

#define DEFAULT_RCPATH  "/reg/g/pcds/dist/pds/amo/misc/.offlinereader"

#include <string>
#include <vector>

using namespace Pds;

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

int instrumentPrint(const char *offlinerc, std::string instr, unsigned verbose) {

  const char *partition = instr.c_str();
  OfflineClient* offlineclient;

  offlineclient = new OfflineClient(offlinerc, partition, 0u, (verbose > 1));

  if (offlineclient) {
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
    delete offlineclient;
  } else {
    fprintf(stderr,"%s: failed to instantiate OfflineClient\n", __FUNCTION__);
  }

  return (0);
}

int instrumentParse(char *inbuf, char **inst, int *station)
{
  char *saveptr, *token1, *token2;
  token1 = token2 = NULL;
  static char line[MAXLINE+1];
  int rv = 0;

  strncpy(line, inbuf, MAXLINE);
  token1 = strtok_r(line, ":", &saveptr);
  if (token1) {
    *inst = token1;
    while (*token1) {
      *token1 = toupper(*token1);
      token1++;
    }
    token2 = strtok_r(NULL, ":", &saveptr);
    if (token2) {
      if (sscanf(token2, "%d", station) != 1) {
        rv = 1; // error
      }
    } else {
      *station = DEFAULT_STATION;
    }
  } else {
    rv = 1; // error
  }
  return (rv);
}

int main(int argc, char* argv[]) {
  int ii;
  int parseErr = 0;
  char *inst = "";
  int station = -1;
  char *rcpath = DEFAULT_RCPATH;
  unsigned verbose = 0;
  std::vector<std::string> instList;
  std::vector<std::string>::iterator it;

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
      if (instrumentParse(argv[ii], &inst, &station) != 0) {
        parseErr++;
        break;
      } else {
        instList.push_back(inst);
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

  for (it = instList.begin(); it != instList.end(); it++) {
    instrumentPrint(rcpath, *it, verbose);
  }

  return 0;
}
