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
#include <tuple>
#include <chrono>

static const char sNsCamTestVersion[] = "1.0";

static void showVersion(const char* p)
{
  printf( "Version:  %s  Ver %s\n", p, sNsCamTestVersion );
}

static bool parsePot(const std::string& arg, std::string& name, double& v, bool& tune)
{
  size_t pos = arg.find(",");
  if (pos != std::string::npos) {
    name = arg.substr(0, pos);
    std::string valuestr = arg.substr(pos+1);
    pos = valuestr.find(",");
    if (pos != std::string::npos) {
      std::string tunestr = valuestr.substr(pos+1);
      std::string valuestr = valuestr.substr(0, pos);
      return Pds::CmdLineTools::parseDouble(valuestr.c_str(), v) && Pds::CmdLineTools::parseBool(tunestr.c_str(), tune);
    } else {
      tune = false;
      return Pds::CmdLineTools::parseDouble(valuestr.c_str(), v);
    }
  } else {
    return false;
  }
}

static void showUsage(const char* p)
{
  printf("Usage: %s [-v|--version] [-h|--help]\n"
         "[-w|--write <filename prefix>] [-n|--number <number of images>] [-t|--timing <ton>,<toff>[,tdel]]\n"
         "[-p|--pot <name>,<val>] [-o|--osc <0,1,2,3>] -H|--host <host> [-P|--port <portset>] [-T|--trigger]\n"
         " Options:\n"
         "    -w|--write    <filename prefix>         output filename prefix\n"
         "    -n|--number   <number of images>        number of images to be captured (default: 1)\n"
         "    -t|--timing   <ton>,<toff>[,tdel]       the timing setup to use for the UXI detector (default: 2,2,0)\n"
         "    -P|--port     <port>                    set the UXI detector server port (default: 20482)\n"
         "    -H|--host     <host>                    set the UXI detector server host ip\n"
         "    -p|--pot      <name>,<val>[,tune]       the named pot/dac to the specified value\n"
         "    -m|--max      <max>                     the maximum number of pixel values to print for each frame (default: 0)\n"
         "    -r|--roi      <r0,r1,f0,f1>             set an ROI with first/last row and then frame\n"
         "    -o|--osc      <0,1,2,3>                 set the oscillator mode (default: 0)\n"
         "    -T|--trigger                            use external trigger\n"
         "    -l|--list                               list discoverable devices\n"
         "    -d|--debug    <level>                   Set debug level (default: 1)\n"
         "    -v|--version                            show file version\n"
         "    -h|--help                               print this message and exit\n", p);
}

using namespace Pds::NsCam;

static const Logger::Level logLevels[] = {
  Logger::Level::ERROR,
  Logger::Level::WARN,
  Logger::Level::INFO,
  Logger::Level::DEBUG,
};
static const size_t numLogLevels = sizeof(logLevels) / sizeof(logLevels[0]);

static const OscillatorType oscTypes[] = {
  OscillatorType::RELAXATION,
  OscillatorType::RING,
  OscillatorType::RINGNOOSC,
  OscillatorType::EXTERNAL,
};
static const size_t numOscTypes = sizeof(oscTypes) / sizeof(oscTypes[0]);

int main(int argc, char *argv[]) {
  const char*         strOptions  = ":vhw:n:t:P:H:p:m:r:o:Tld:";
  const struct option loOptions[] =
  {
    {"version",     0, 0, 'v'},
    {"help",        0, 0, 'h'},
    {"write",       1, 0, 'w'},
    {"number",      1, 0, 'n'},
    {"timing",      1, 0, 't'},
    {"port",        1, 0, 'P'},
    {"host",        1, 0, 'H'},
    {"pot",         1, 0, 'p'},
    {"max",         1, 0, 'm'},
    {"roi",         1, 0, 'r'},
    {"osc",         1, 0, 'o'},
    {"trigger",     0, 0, 'T'},
    {"list",        0, 0, 'l'},
    {"debug",       1, 0, 'd'},
    {0,             0, 0,  0 }
  };

  int status = 0;
  unsigned port = 20482;
  unsigned num_images = 1;
  Timing timing{2, 2, 0};
  TriggerType trig_mode = TriggerType::SOFTWARE;
  unsigned max_display = 0;
  unsigned first_row = 0;
  unsigned last_row = 1023;
  unsigned first_frame = 0;
  unsigned last_frame = 3;
  unsigned debug = 1;
  double potvalue = 0.0;
  bool pottune = false;
  unsigned oscillator = 0;
  bool use_roi = false;
  bool list_devices = false;
  bool lUsage = false;
  char* filename = (char *)NULL;
  char* file_prefix = (char *)NULL;
  std::string hostname = "";
  std::string potname = "";
  std::map<std::string, std::pair<double, bool>> pot_config;

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
      case 'w':
        file_prefix = optarg;
        break;
      case 'n':
        if (!Pds::CmdLineTools::parseUInt(optarg,num_images)) {
          printf("%s: option `-n' parsing error\n", argv[0]);
          lUsage = true;
        }
        break;
      case 't':
        switch (Pds::CmdLineTools::parseUInt(optarg,timing.open,timing.closed,timing.delay)) {
          case 2:
          case 3:
            break;
          default:
            printf("%s: option `-t' parsing error\n", argv[0]);
            lUsage = true;
            break;
        }
        break;
      case 'H':
        hostname = optarg;
        break;
      case 'P':
        if (!Pds::CmdLineTools::parseUInt(optarg,port)) {
          printf("%s: option `-P' parsing error\n", argv[0]);
          lUsage = true;
        }
        break;
      case 'p':
        if (!parsePot(optarg, potname, potvalue, pottune)) {
          printf("%s: option `-p' parsing error\n", argv[0]);
          lUsage = true;
        } else {
          pot_config.emplace(std::piecewise_construct,
                             std::forward_as_tuple(potname),
                             std::forward_as_tuple(potvalue, pottune));
        }
        break;
      case 'm':
        if (!Pds::CmdLineTools::parseUInt(optarg,max_display)) {
          printf("%s: option `-m' parsing error\n", argv[0]);
          lUsage = true;
        }
        break;
      case 'r':
        if (Pds::CmdLineTools::parseUInt(optarg,first_row,last_row,first_frame,last_frame)!=4) {
          printf("%s: option `-r' parsing error\n", argv[0]);
          lUsage = true;
        } else {
          use_roi = true;
        }
        break;
      case 'o':
        if (!Pds::CmdLineTools::parseUInt(optarg,oscillator)) {
          printf("%s: option `-o' parsing error\n", argv[0]);
          lUsage = true;
        } else if (oscillator >= numOscTypes) {
          printf("%s: option `-o' value out of range\n", argv[0]);
          lUsage = true;
        }
        break;
      case 'T':
        trig_mode = TriggerType::HARDWARE;
        break;
      case 'l':
        list_devices = true;
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

  if (!list_devices && hostname.empty()) {
    printf("%s: hostname is required\n", argv[0]);
    lUsage = true;
  }

  if (lUsage) {
    showUsage(argv[0]);
    return 1;
  }

  if (file_prefix) {
    filename = new char[strlen(file_prefix) + 16];
  }

  // initialize nscam logger
  Logger::instance().setLevel(logLevels[debug]);

  // just list discoverable devices and exit
  if (list_devices) {
    Pds::NsCam::Detector::listDevices();
    return 0;
  }

  try {
    printf("Initializing the detector:\n");
    printf("==========================\n");
    printf(" Attempting to connect to the detector at %s:%u\n", hostname.c_str(), port);
    // time the startup time of the camera
    auto start = std::chrono::steady_clock::now();
    Detector det(hostname, port, CommType::GIGE, BoardType::LLNL_V1, SensorType::ICARUS2);
    auto end = std::chrono::steady_clock::now();
    std::chrono::duration<double> elapsed_seconds = end - start;
    printf(" Initialized the detector in %f seconds\n\n", elapsed_seconds.count());

    printf("Configuring the detector:\n");
    printf("=========================\n");
    start = std::chrono::steady_clock::now();
    if (use_roi) {
      printf(" Setting ROI rows (first, last): %u %u\n", first_row, last_row);
      det.setRows(first_row, last_row);
      printf(" Setting ROI frames (first, last): %u %u\n", first_frame, last_frame);
      det.setFrames(first_frame, last_frame);
    } else {
      det.setRows();
      det.setFrames();
    }

    for (const auto& kv : pot_config) {
      printf(" Setting %s to %.3f V%s\n", kv.first.c_str(), kv.second.first, kv.second.second ? " tuned" : "");
      det.setPotV(kv.first, kv.second.first, kv.second.second);
    }
    printf(" Setting timing for both sides to open %u, closed %u, delay %u\n",
           timing.open, timing.closed, timing.delay);
    det.setTiming(SideType::AB, timing);
    printf(" Setting the oscillator to %s\n", toString(oscTypes[oscillator]));
    det.setOscillator(oscTypes[oscillator]);
    end = std::chrono::steady_clock::now();
    elapsed_seconds = end - start;
    printf(" Configured the detector in %f seconds\n\n", elapsed_seconds.count());

    det.interfaceInfo();
    det.boardInfo();
    det.sensorInfo();
    det.statusInfo();
    det.potInfo();

    size_t num_pixels = det.npixels();
    if (num_pixels > 0 && num_images > 0) {
      printf("Acquiring %u images from the detector.\n", num_images);
      start = std::chrono::steady_clock::now();
      for (size_t i=0; i<num_images; i++) {
        std::unique_ptr<uint16_t[]> frame = det.waitFrame16(trig_mode);
        printf("Received frame acquisition %zu", i+1);
        if (max_display > 0) {
          printf(":\n");
          for (size_t j=0; j<(max_display>num_pixels ? num_pixels : max_display); j++) {
            printf(" %u", frame[j]);
          }
        }
        printf("\n");
        if (file_prefix) {
          sprintf(filename, "%s_%04zu.raw", file_prefix, i+1);
          printf("writing image %zu to file: %s\n", i+1, filename);
          FILE *f = fopen(filename, "wb");
          fwrite(frame.get(), sizeof(uint16_t), num_pixels, f);
          fclose(f);
        }
      }
      end = std::chrono::steady_clock::now();
      elapsed_seconds = end - start;
      printf("Acquired %u images in %f seconds - %f Hz frame rate\n",
             num_images,
             elapsed_seconds.count(),
             num_images / elapsed_seconds.count());
    } else if (num_pixels == 0) {
      printf("Failed to determine number of pixels in detector!\n");
      status = 1;
    }
  } catch(const NsCamException& e) {
    printf("Exception encountered from the detector: %s\n", e.what());
    status = 1;
  }

  if (filename) delete[] filename;

  return status;
}
