#include <stdio.h>
#include <getopt.h>
#include <string>

#include "pds/config/EvrConfigType.hh"
#include "pds/config/CfgClientNfs.hh"
#include "pdsapp/config/Experiment.hh"
#include "pdsapp/config/Table.hh"

const char sSqlDbTestVersion[] = "0.1";

using namespace std;

int sqlDbTest(string sConfigPath)
{
  int runKey;
  string sConfigType = "BEAM";
  try
  { Pds_ConfigDb::Experiment expt(sConfigPath.c_str(),
                                  Pds_ConfigDb::Experiment::NoLock);
    expt.read();
    const Pds_ConfigDb::TableEntry* entry = expt.table().get_top_entry(sConfigType);
    if (entry == NULL)
    {
      printf("sqlDbTest(): Invalid config db path [%s] type [%s]\n",sConfigPath.c_str(), sConfigType.c_str());
      return 1;
    }
    runKey = strtoul(entry->key().c_str(),NULL,16);
  }
  catch (const std::exception& except)
  {
    printf("sqlDbTest(): Experiment read failed: %s\n", except.what());
    return 2;
  }

  return 0;
}


static void showUsage()
{
  printf( "Usage:  sqlDbTest  [-v|--version] [-h|--help] [-d|--db <db_file>]"
          "  Options:\n"
          "    -v|--version                 Show file version.\n"
          "    -h|--help                    Show usage.\n"
          "    -d|--db   <db_path>          Set db path\n"
        );
}

static void showVersion()
{
  printf( "Version:  sqlDbTest  Ver %s\n", sSqlDbTestVersion );
}

int main(int argc, char* argv[])
{
  const char*   strOptions    = ":vhd:";
  const struct option loOptions[]   =
  {
    {"ver",      0, 0, 'v'},
    {"help",     0, 0, 'h'},
    {"db",       1, 0, 'd'},
    {0,          0, 0,  0 }
  };

  // parse the command line for our boot parameters
  string            sConfigDb;
  string            sConfigType = "BEAM";

  int               iOptionIndex  = 0;
  while ( int opt = getopt_long(argc, argv, strOptions, loOptions, &iOptionIndex ) )
  {
    if ( opt == -1 ) break;

    switch(opt)
    {
      case 'v':               /* Print usage */
        showVersion();
        return 0;
      case 'd':
        sConfigDb = optarg;
        break;
      case '?':               /* Terse output mode */
        if (optopt)
          printf( "sqlDbTest:main(): Unknown option: %c\n", optopt );
        else
          printf( "sqlDbTest:main(): Unknown option: %s\n", argv[optind-1] );
        break;
      case ':':               /* Terse output mode */
        printf( "sqlDbTest:main(): Missing argument for %c\n", optopt );
        break;
      default:
      case 'h':               /* Print usage */
        showUsage();
        return 0;
    }
  }

  argc -= optind;
  argv += optind;

  sqlDbTest(sConfigDb);
}
