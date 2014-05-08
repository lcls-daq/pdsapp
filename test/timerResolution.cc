#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <signal.h>
#include <time.h>
#include "pds/oceanoptics/histreport.hh"

const char* sVersion = "0.01";

class timerResolution {
public:
  timerResolution(int niter, int sleepMsec, int minItv, int maxItv, int div) :
    niter(niter), sleepMsec(sleepMsec), minItv(minItv), maxItv(maxItv), div(div) {}

  int run() {
    timeval timeSleepMicroOrg = {0, sleepMsec}; // 1 milliseconds

    bool       bFirstTime    = true;
    timespec   tsPrev        = {0,0};
    timespec   tsPrevHr      = {0,0};
    timespec   tsPrevMono    = {0,0};
    timespec   tsPrevMonoHr  = {0,0};
    timespec   tsPrevProc    = {0,0};
    timespec   tsPrevThread  = {0,0};

    HistReport   histDiffTs(minItv,maxItv,(maxItv-minItv) / (double)div);

    for (int i=0; i<niter+1; ++i) {
      timespec          ts, tsHr, tsMono, tsMonoHr, tsThread, tsProc;
      clock_gettime(CLOCK_REALTIME, &ts);
      //clock_gettime(CLOCK_REALTIME_HR, &tsHr);
      clock_gettime(CLOCK_MONOTONIC, &tsMono);
      //clock_gettime(CLOCK_MONOTONIC_HR, &tsMonoHr);
      clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &tsProc);
      clock_gettime(CLOCK_THREAD_CPUTIME_ID, &tsThread);

      if (!bFirstTime) {
        int64_t i64DiffTs = (ts.tv_sec - tsPrev.tv_sec) * 1000000000LL + ts.tv_nsec - tsPrev.tv_nsec;
        //printf("[%04d] diff %012Ld\n", i, i64DiffTs);

        if (sleepMsec == 0 && i64DiffTs == 0) {
          --i;
          continue;
        }

        histDiffTs.addValue(i64DiffTs);
      }
      else
        bFirstTime = false;

      tsPrev      = ts;
      tsPrevHr    = tsHr;
      tsPrevMono  = tsMono;
      tsPrevMonoHr= tsMonoHr;
      tsPrevProc  = tsProc;
      tsPrevThread= tsThread;

      if (sleepMsec > 0) {
        // This data will be modified by select(), so need to be reset
        timeval timeSleepMicro = timeSleepMicroOrg;
        // use select() to simulate nanosleep(), because experimentally select() controls the sleeping time more precisely
        select( 0, NULL, NULL, NULL, &timeSleepMicro);
      }
    }

    histDiffTs.report("Diff Ts");
    return 0;
  }

private:
  int niter;
  int sleepMsec;
  int minItv;
  int maxItv;
  int div;
};

void showUsage(char* progname)
{
  printf(
    "Usage:  %s  [-v|--version] [-h|--help] [-n|--niter <num>] [-s|--sleep <msec>]\n"
    "            [--max <max>] [--min <min>] [--div <num>]\n"
    "  Options: [default value]\n"
    "    -v|--version                      Show file version\n"
    "    -h|--help                         Show usage\n"
    "    -n|--numiter  <num>               Run <num> iterations. [1000]\n"
    "    -s|--sleep    <msec>              Sleep for <msec> micro seconds between iteration. [10]\n"
    "       --min      <min>               Min interval for profiling. [0] \n"
    "       --max      <max>               Max interval for profiling. [100000000]\n"
    "       --div      <num>               Report results based on <num> divisions. [100]\n"
    ,
    progname
  );
}

void showVersion(char* progname)
{
  printf( "Version:  %s  Ver %s\n", progname, sVersion );
}

int giExitAll = 0;
void signalIntHandler(int iSignalNo)
{
  printf( "\nsignalIntHandler(): signal %d received. Stopping all activities\n", iSignalNo );
  giExitAll = 1;
  exit(1);
}

int main(int argc, char **argv)
{
  const char*         strOptions  = ":vhn:s:";
  const struct option loOptions[] =
  {
    {"ver",      0, 0, 'v'},
    {"help",     0, 0, 'h'},
    {"niter",    1, 0, 'n'},
    {"sleep",    1, 0, 's'},
    {"min",      1, 0, 1001},
    {"max",      1, 0, 1002},
    {"div",      1, 0, 1003},
    {0,          0, 0,  0 }
  };

  int     niter        = 1000;
  int     sleepMsec    = 10;
  int     minItv       = 0;
  int     maxItv       = 100000000;
  int     div          = 100;

  int iOptionIndex = 0;
  while ( int opt = getopt_long(argc, argv, strOptions, loOptions, &iOptionIndex ) )
  {
    if ( opt == -1 ) break;

    switch(opt)
    {
      case 'v':               /* Print usage */
        showVersion(argv[0]);
        return 0;
      case 'n':
        niter       = strtol(optarg, NULL, 0);
        break;
      case 's':
        sleepMsec   = strtol(optarg, NULL, 0);
        break;
      case 1001:
        minItv   = strtol(optarg, NULL, 0);
        break;
      case 1002:
        maxItv   = strtol(optarg, NULL, 0);
        break;
      case 1003:
        div     = strtol(optarg, NULL, 0);
        break;
      case '?':               /* Terse output mode */
        printf( "%s:main(): Unknown option: %c\n", argv[0], optopt );
        break;
      case ':':               /* Terse output mode */
        printf( "%s:main(): Missing argument for %c\n", argv[0], optopt );
        break;
      default:
      case 'h':               /* Print usage */
        showUsage(argv[0]);
        return 0;

    }
  }

  argc -= optind;
  argv += optind;

  /*
   * Register singal handler
   */
  struct sigaction sigActionSettings;
  sigemptyset(&sigActionSettings.sa_mask);
  sigActionSettings.sa_handler = signalIntHandler;
  sigActionSettings.sa_flags   = SA_RESTART;

  if (sigaction(SIGINT, &sigActionSettings, 0) != 0 )
    printf( "main(): Cannot register signal handler for SIGINT\n" );
  if (sigaction(SIGTERM, &sigActionSettings, 0) != 0 )
    printf( "main(): Cannot register signal handler for SIGTERM\n" );

  timerResolution( niter, sleepMsec, minItv, maxItv, div ).run();
  return 0;
}
