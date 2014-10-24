#include "pds/config/XtcClient.hh"
#include "pds/service/SysClk.hh"
#include "pdsdata/xtc/DetInfo.hh"
#include "pdsdata/xtc/TypeId.hh"

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>

using namespace Pds;
using Pds_ConfigDb::XtcClient;

static void showUsage(const char* p)
{
  printf("Usage: %s --db <db path>  --key <key> --src <src> --type <typeId>\n",
         p);
}

int main(int argc, char* argv[])
{
  const char* db_path = 0;
  int key = 0;
  DetInfo  src("");
  TypeId   typeId("");

  while (1) {
    int option_index = 0;
    static struct option long_options[] = {
      {"db"  , 1, 0, 'd'},
      {"key" , 1, 0, 'k'},
      {"src" , 1, 0, 's'},
      {"type", 1, 0, 't'},
      {"help", 0, 0, 'h'},
      {0, 0, 0, 0}
    };
    int c = getopt_long(argc, argv, "h", long_options, &option_index);
    if (c == -1) {
      break;
    }
    switch (c) {
    case 'd':
      db_path = optarg;
      break;
    case 'k':
      key = strtoul(optarg,NULL,0);
      break;
    case 's':
      src = DetInfo(optarg);
      break;
    case 't':
      typeId = TypeId(optarg);
      break;
    case 'h':
      showUsage(argv[0]);
      exit(1);
    default:
      printf("unrecognized argument\n");
      showUsage(argv[0]);
      exit(-1);
    };
  };

  if (!db_path)
    printf("No database path given\n");
  else if (key<0)
    printf("No key given\n");
  else if (src.detector()==DetInfo::NumDetector)
    printf("No valid src given  [<Detector>-<Index>|<Device>-<Index>]\n");
  else if (typeId.id()==TypeId::NumberOf)
    printf("No valid type given [<Type>_v<Index>]\n");
  else {
    const unsigned maxSize = 0x100000;
    char* dst = new char[maxSize];

    XtcClient* db = XtcClient::open(db_path);

    struct timespec tv_b;
    clock_gettime(CLOCK_REALTIME,&tv_b);

    int result = db->getXTC(key,
			    src,
			    typeId,
			    dst,
			    maxSize);

    struct timespec tv_e;
    clock_gettime(CLOCK_REALTIME,&tv_e);

    long long int dt = SysClk::diff(tv_e,tv_b);
    printf("CfgClientNfs::fetch() %lld.%09lld s [%lld]\n", dt/1000000000LL, dt%1000000000LL, dt);

    delete[] dst;
  }
}
