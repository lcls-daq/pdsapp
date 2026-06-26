#include "pds/nscam/Detector.hh"
#include "pds/nscam/Error.hh"
#define _NOLOGGERMACROS 1
#include "pds/nscam/Logger.hh"
#include "pds/service/CmdLineTools.hh"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>

#include <string>
#include <map>
#include <chrono>

static const char sNsCamTimingVersion[] = "1.0";

static void showVersion(const char* p)
{
  printf( "Version:  %s  Ver %s\n", p, sNsCamTimingVersion );
}

static void showUsage(const char* p)
{
  printf("Usage: %s [-v|--version] [-h|--help]\n"
         "-H|--host <host> [-P|--port <portset>]\n"
         " Options:\n"
         "    -P|--port     <port>                    set the UXI detector server port (default: 20482)\n"
         "    -H|--host     <host>                    set the UXI detector server host ip\n"
         "    -d|--debug    <level>                   Set debug level (default: 0)\n"
         "    -v|--version                            show file version\n"
         "    -h|--help                               print this message and exit\n", p);
}

using namespace Pds::NsCam;

struct TestCase {
  Timing cmd;
  Timing timing;
  Sequence seq;
  uint32_t reghi;
  uint32_t reglo;
  bool fail;
};

static std::vector<TestCase> TEST_CASES = {
  {{2, 2, 0}, {2, 2, 0}, {0, 2, 2, 2, 2, 2, 2, 2}, 0x00000066, 0x66666666, false},
  {{2, 2, 1}, {2, 2, 1}, {1, 2, 2, 2, 2, 2, 2, 2}, 0x000000cc, 0xcccccccc, false},
  {{3, 2, 0}, {3, 2, 0}, {0, 3, 2, 3, 2, 3, 2, 3}, 0x00000073, 0x9ce739ce, false},
  {{3, 2, 10}, {3, 2, 10}, {10, 3, 2, 3, 2, 3, 2, 3}, 0x00000073, 0x9ce73800, false},
  {{39, 1, 0}, {39, 1, 0}, {0, 39, 1, 39, 1, 39, 1, 39}, 0x000000ff, 0xfffffffe, false},
  {{39, 1, 1}, {39, 1, 0}, {0, 39, 1, 39, 1, 39, 1, 39}, 0x000000ff, 0xfffffffe, true},
  {{38, 2, 0}, {38, 2, 0}, {0, 38, 2, 38, 2, 38, 2, 38}, 0x0000007f, 0xfffffffe, false},
  {{38, 2, 1}, {38, 2, 0}, {0, 38, 2, 38, 2, 38, 2, 38}, 0x0000007f, 0xfffffffe, true},
  {{36, 2, 2}, {36, 4, 2}, {2, 36, 4, 36, 4, 36, 4, 36}, 0x0000007f, 0xfffffff8, false},
  {{10, 2, 0}, {10, 2, 0}, {0, 10, 2, 10, 2, 10, 6, 10}, 0x00000007, 0xfe7fe7fe, false},
  {{10, 9, 0}, {10, 9, 0}, {0, 10, 9, 10, 11, 10, 9, 10}, 0x00000000, 0x3ff007fe, false},
  {{10, 9, 1}, {10, 9, 1}, {1, 10, 9, 10, 11, 10, 9, 10}, 0x00000000, 0x7fe00ffc, false},
  {{15, 10, 0}, {15, 10, 0}, {0, 15, 10, 14, 1, 15, 10, 14}, 0x000000ff, 0xfc00fffe, false},
  {{15, 10, 1}, {15, 25, 1}, {1, 15, 25, 15, 25, 15, 25, 15}, 0x00000000, 0x0001fffc, false},
  {{15, 9, 0}, {15, 9, 0}, {0, 15, 9, 15, 1, 15, 9, 15}, 0x000000ff, 0xfe00fffe, false},
  {{14, 10, 0}, {14, 10, 0}, {0, 14, 10, 14, 2, 14, 10, 14}, 0x0000007f, 0xfe007ffe, false},
  {{14, 10, 1}, {14, 10, 1}, {1, 14, 10, 14, 2, 14, 10, 14}, 0x000000ff, 0xfc00fffc, false},
  {{14, 10, 2}, {14, 10, 2}, {2, 14, 10, 13, 3, 14, 10, 13}, 0x000000ff, 0xf801fff8, false},
  {{20, 2, 0}, {20, 20, 0}, {0, 20, 20, 20, 20, 20, 20, 20}, 0x00000000, 0x001ffffe, false},
  {{20, 2, 2}, {20, 20, 2}, {2, 20, 20, 20, 20, 20, 20, 20}, 0x00000000, 0x007ffff8, false},
  {{19, 19, 0}, {19, 21, 0}, {0, 19, 21, 19, 21, 19, 21, 19}, 0x00000000, 0x000ffffe, false},
  {{19, 19, 1}, {19, 21, 1}, {1, 19, 21, 19, 21, 19, 21, 19}, 0x00000000, 0x001ffffc, false},
  {{19, 19, 2}, {19, 21, 2}, {2, 19, 21, 19, 21, 19, 21, 19}, 0x00000000, 0x003ffff8, false},
  {{19, 20, 0}, {19, 21, 0}, {0, 19, 21, 19, 21, 19, 21, 19}, 0x00000000, 0x000ffffe, false},
  {{19, 20, 1}, {19, 21, 1}, {1, 19, 21, 19, 21, 19, 21, 19}, 0x00000000, 0x001ffffc, false},
  {{19, 21, 0}, {19, 21, 0}, {0, 19, 21, 19, 21, 19, 21, 19}, 0x00000000, 0x000ffffe, false},
  {{19, 21, 1}, {19, 21, 0}, {0, 19, 21, 19, 21, 19, 21, 19}, 0x00000000, 0x000ffffe, true},
  {{15, 25, 0}, {15, 25, 0}, {0, 15, 25, 15, 25, 15, 25, 15}, 0x00000000, 0x0000fffe, false},
};

static const Logger::Level logLevels[] = {
  Logger::Level::OFF,
  Logger::Level::ERROR,
  Logger::Level::WARN,
  Logger::Level::INFO,
  Logger::Level::DEBUG,
};
static const size_t numLogLevels = sizeof(logLevels) / sizeof(logLevels[0]);

int main(int argc, char *argv[]) {
  const char*         strOptions  = ":vhP:H:d:";
  const struct option loOptions[] =
  {
    {"version",     0, 0, 'v'},
    {"help",        0, 0, 'h'},
    {"port",        1, 0, 'P'},
    {"host",        1, 0, 'H'},
    {"debug",       1, 0, 'd'},
    {0,             0, 0,  0 }
  };

  bool lUsage = false;
  int status = 0;
  unsigned port = 20482;
  unsigned debug = 0;
  std::string hostname = "";

  int optionIndex  = 0;
  while ( int opt = getopt_long(argc, argv, strOptions, loOptions, &optionIndex ) ) {
    if ( opt == -1 ) break;

    switch(opt) {
      case 'h':               /* Print usage */
        showUsage(argv[0]);
        return 0;
      case 'v':               /* Print version */
        showVersion(argv[0]);
        return 0;
      case 'H':
        hostname = optarg;
        break;
      case 'P':
        if (!Pds::CmdLineTools::parseUInt(optarg,port)) {
          printf("%s: option `-P' parsing error\n", argv[0]);
          lUsage = true;
        }
        break;
      case 'd':
        if (!Pds::CmdLineTools::parseUInt(optarg,debug)) {
          printf("%s: option `-d' parsing error\n", argv[0]);
          lUsage = true;
        } else if (debug >= numLogLevels) {
          printf("%s: option `-d' value out of range\n", argv[0]);
          lUsage = true;
        }
        break;
      case ':':
        printf("%s: Missing argument for %c\n", argv[0], optopt);
      default:
        lUsage = true;
        break;
    }
  }

  if (optind < argc) {
    printf("%s: invalid argument -- %s\n", argv[0], argv[optind]);
    lUsage = true;
  }

  if (hostname.empty()) {
    printf("%s: hostname is required\n", argv[0]);
    lUsage = true;
  }

  if (lUsage) {
    showUsage(argv[0]);
    return 1;
  }

  // initialize nscam logger
  Logger::instance().setLevel(logLevels[debug]);

  try {
    // time the startup time of the camera
    auto start = std::chrono::steady_clock::now();
    Detector det(hostname, port, CommType::GIGE, BoardType::LLNL_V1, SensorType::ICARUS2);
    auto end = std::chrono::steady_clock::now();
    std::chrono::duration<double> elapsed_seconds = end - start;
    printf("Initialized the detector in %f seconds\n\n", elapsed_seconds.count());

    size_t ntests = 0;
    size_t nfails = 0;

    printf("Testing the detector:\n");
    printf("=========================\n");
    start = std::chrono::steady_clock::now();
    for (const auto& tc : TEST_CASES) {
      bool failed = false;
      printf("Case %zu:\n", ntests);
      printf(" Setting timing to %s\n", Timing::toString(tc.cmd).c_str());
      try {
        printf(" - Checking pass/fail of setTiming: ");
        det.setTiming(SideType::A, tc.cmd);
        if (tc.fail) {
          printf("\033[31mfailed - expected an exception to be thrown!\033[0m\n");
          failed = true;
        } else {
          printf("\033[32mpassed\033[0m\n");
        }
      } catch(const InvalidTiming& timing_err) {
        if (tc.fail) {
          printf("\033[32mpassed\033[0m\n");
        } else {
          printf("\033[31mfailed - unexpected exception thrown: %s\033[0m\n", timing_err.what());
          failed = true;
        }
      }
      Timing timing = det.getTiming(SideType::A);
      printf(" - Checking the return of getTiming: ");
      if (timing != tc.timing) {
        printf("\033[31mfailed - %s vs expected %s\033[0m\n", Timing::toString(timing).c_str(), Timing::toString(tc.timing).c_str());
        failed = true;
      } else {
        printf("\033[32mpassed\033[0m\n");
      }
      Sequence seq = det.getActualTiming(SideType::A);
      printf(" - Checking the return of getActualTiming: ");
      if (seq != tc.seq) {
        printf("\033[31mfailed - %s vs expected %s\033[0m\n", toString(seq).c_str(), toString(tc.seq).c_str());
        failed = true;
      } else {
        printf("\033[32mpassed\033[0m\n");
      }
      uint32_t reghi = det.getRegister("HS_TIMING_DATA_AHI");
      uint32_t reglo = det.getRegister("HS_TIMING_DATA_ALO");
      printf(" - Checking registers: ");
      if (reghi != tc.reghi || reglo != tc.reglo) {
        printf("\033[31mfailed - %08x, %08x vs expected %08x, %08x\033[0m\n", reghi, reglo, tc.reghi, tc.reglo);
        failed = true;
      } else {
        printf("\033[32mpassed\033[0m\n");
      }
      printf("-------------------------\n");
      ntests++;
      if (failed) nfails++;
    }
    printf("\nTested the detector in %f seconds\n", elapsed_seconds.count());
    printf("\nPassed %zu out of %zu tests!\n", ntests - nfails, ntests);
  } catch(const NsCamException& err) {
    printf("Exception encountered from the detector: %s\n", err.what());
    status = 1;
  }

  return status;
}
