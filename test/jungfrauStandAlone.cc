#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <getopt.h>
#include <signal.h>
#include "pds/jungfrau/Driver.hh"

#include <vector>

static const char sJungfrauTestVersion[] = "1.0";

static volatile sig_atomic_t running = 1;

static void sigHandler(int signal)
{
  psignal(signal, "running stopped on signal");
  running = 0;
}

static void showVersion(const char* p)
{
  printf( "Version:  %s  Ver %s\n", p, sJungfrauTestVersion );
}

static void showUsage(const char* p)
{
  printf("Usage: %s [-v|--version] [-h|--help]\n"
         "[-w|--write <filename prefix>] [-n|--number <number of images>] [-e|--exposure <exposure time (sec)>]\n"
         "[-b|--bias <bias>] [-g|--gain <gain>] [-S|--speed <speed>] [-t|--trigger <delay>] [-r|--receiver]\n"
         "[-i|--info] [-f|--flowctrl]\n"
         "-H|--host <host> [-P|--port <port>] -m|--mac <mac> -d|--detip <detip> -s|--sls <sls>\n"
         " Options:\n"
         "    -w|--write    <filename prefix>         output filename prefix\n"
         "    -n|--number   <number of images>        number of images to be captured (default: 1)\n"
         "    -e|--exposure <exposure time>           exposure time (sec) (default: 0.00001 sec)\n"
         "    -E|--period   <exposure period>         exposure period (sec) (default: 0.00001 sec)\n"
         "    -b|--bias     <bias>                    the bias voltage to apply to the sensor in volts (default: 200)\n"
         "    -g|--gain     <gain 0-5>                the gain mode of the detector (default: Normal - 0)\n"
         "    -S|--speed    <speed 0-1>               the clock speed mode of the detector (default: Half - 1)\n"
         "    -P|--port     <port>                    set the receiver udp port number (default: 32410)\n"
         "    -H|--host     <host>                    set the receiver host ip\n"
         "    -m|--mac      <mac>                     set the receiver mac address\n"
         "    -d|--detip    <detip>                   set the detector ip address\n"
         "    -s|--sls      <sls>                     set the hostname of the slsDetector interface\n"
         "    -t|--trigger  <delay>                   the internal acquisition start delay to use with an external trigger is seconds (default: 0.000238)\n"
         "    -T|--external                           use an external trigger for acquistion (default: false)\n"
         "    -M|--threaded                           use the multithreaded version of the Jungfrau detector driver (default: false)\n"
         "    -r|--receiver                           do not attempt to configure ip settings of the receiver (default: true)\n"
         "    -i|--info                               display additional info about recieved frames (default: false)\n"
         "    -f|--flowctrl                           disable flow control for udp interface (default: true)\n"
         "    -v|--version                            show file version\n"
         "    -h|--help                               print this message and exit\n", p);
}

int main(int argc, char **argv)
{
  const char*         strOptions  = ":vhw:n:e:E:b:g:S:P:H:m:d:s:t:TMrif";
  const struct option loOptions[] =
  {
    {"ver",         0, 0, 'v'},
    {"help",        0, 0, 'h'},
    {"write",       1, 0, 'w'},
    {"number",      1, 0, 'n'},
    {"exposure",    1, 0, 'e'},
    {"period",      1, 0, 'E'},
    {"bias",        1, 0, 'b'},
    {"gain",        1, 0, 'g'},
    {"speed",       1, 0, 'S'},
    {"port",        1, 0, 'P'},
    {"host",        1, 0, 'H'},
    {"mac",         1, 0, 'm'},
    {"detip",       1, 0, 'd'},
    {"sls",         1, 0, 's'},
    {"trigger",     1, 0, 't'},
    {"external",    0, 0, 'T'},
    {"threaded",    0, 0, 'M'},
    {"receiver",    0, 0, 'r'},
    {"info",        0, 0, 'i'},
    {"flowctrl",    0, 0, 'f'},
    {0,             0, 0,  0 }
  };

  unsigned port  = 32410;
  unsigned num_modules = 0;
  int numImages = 1;
  unsigned bias = 200;
  double exposureTime = 0.00001;
  double exposurePeriod = 0.2;
  double triggerDelay = 0.000238;
  double tsClock = 10.e6;
  bool lUsage = false;
  bool configReceiver = true;
  bool external = false;
  bool threaded = false;
  bool show_info = false;
  bool use_flow_ctrl = true;
  unsigned gain_value = 0;
  unsigned speed_value = 1;
  JungfrauConfigType::GainMode gain = JungfrauConfigType::Normal;
  JungfrauConfigType::SpeedMode speed = JungfrauConfigType::Half;
  std::vector<char*> sHost;
  std::vector<char*> sMac;
  std::vector<char*> sDetIp;
  std::vector<char*> sSlsHost;
  char* fileName = (char *)NULL;

  int optionIndex  = 0;
  while ( int opt = getopt_long(argc, argv, strOptions, loOptions, &optionIndex ) ) {
    if ( opt == -1 ) break;

    switch(opt) {
      case 'h':               /* Print usage */
        showUsage(argv[0]);
        return 0;
      case 'v':               /* Print usage */
        showVersion(argv[0]);
        return 0;
      case 'w':
        fileName = new char[strlen(optarg)+6];
        sprintf(fileName, "%s.data", optarg);
        break;
      case 'n':
        numImages = strtoul(optarg, NULL, 0);
        break;
      case 'e':
        exposureTime = strtod(optarg, NULL);
        break;
      case 'E':
        exposurePeriod = strtod(optarg, NULL);
        break;
      case 'b':
        bias = strtoul(optarg, NULL, 0);
        break;
      case 'g':
        gain_value = strtoul(optarg, NULL, 0);
        break;
      case 'S':
        speed_value = strtoul(optarg, NULL, 0);
        break;
      case 'H':
        sHost.push_back(optarg);
        break;
      case 'P':
        port = strtoul(optarg, NULL, 0);
        break;
      case 'm':
        sMac.push_back(optarg);
        break;
      case 'd':
        sDetIp.push_back(optarg);
        break;
      case 's':
        sSlsHost.push_back(optarg);
        num_modules++;
        break;
      case 'r':
        configReceiver = false;
        break;
      case 't':
        triggerDelay = strtod(optarg, NULL);
        break;
      case 'T':
        external = true;
        break;
      case 'M':
        threaded = true;
        break;
      case 'i':
        show_info = true;
        break;
      case 'f':
        use_flow_ctrl = false;
        break;
      case '?':
        if (optopt)
          printf("%s: Unknown option: %c\n", argv[0], optopt);
        else
          printf("%s: Unknown option: %s\n", argv[0], argv[optind-1]);
      case ':':
        printf("%s: Missing argument for %c\n", argv[0], optopt);
      default:
        lUsage = true;
        break;
    }
  }

  if(num_modules == 0) {
    printf("%s: at least one module is required\n", argv[0]);
    lUsage = true;
  }

  if(sHost.size() != num_modules) {
    printf("%s: receiver hostname for each module is required\n", argv[0]);
    lUsage = true;
  }

  if(sMac.size() != num_modules) {
    printf("%s: receiver mac address for each module is required\n", argv[0]);
    lUsage = true;
  }

  if(sDetIp.size() != num_modules) {
    printf("%s: detector ip address for each module is required\n", argv[0]);
    lUsage = true;
  }

  if(sSlsHost.size() != num_modules) {
    printf("%s: slsDetector interface hostname for each module is required\n", argv[0]);
    lUsage = true;
  }

  if (optind < argc) {
    printf("%s: invalid argument -- %s\n", argv[0], argv[optind]);
    lUsage = true;
  }

  // Check that the gain setting is valid
  switch(gain_value) {
    case JungfrauConfigType::Normal:
      gain = JungfrauConfigType::Normal;
      break;
    case JungfrauConfigType::FixedGain1:
      gain = JungfrauConfigType::FixedGain1;
      break;
    case JungfrauConfigType::FixedGain2:
      gain = JungfrauConfigType::FixedGain2;
      break;
    case JungfrauConfigType::ForcedGain1:
      gain = JungfrauConfigType::ForcedGain1;
      break;
    case JungfrauConfigType::ForcedGain2:
      gain = JungfrauConfigType::ForcedGain2;
      break;
    case JungfrauConfigType::HighGain0:
      gain = JungfrauConfigType::HighGain0;
      break;
    default:
      printf("%s: Unknow gain setting: %d\n", argv[0], gain_value);
      printf("Valid choices are:\n");
      printf(" - Normal:      0\n");
      printf(" - FixedGain1:  1\n");
      printf(" - FixedGain2:  2\n");
      printf(" - ForcedGain1: 3\n");
      printf(" - ForcedGain2: 4\n");
      printf(" - HighGain0:   5\n");
      return 1;
  }

  // Check that the speed setting is valid
  switch(speed_value) {
    case JungfrauConfigType::Quarter:
      speed = JungfrauConfigType::Quarter;
      break;
    case JungfrauConfigType::Half:
      speed = JungfrauConfigType::Half;
      break;
    default:
      printf("%s: Unknow speed setting: %d\n", argv[0], speed_value);
      printf("Valid choices are:\n");
      printf(" - Quarter: 0\n");
      printf(" - Half:    1\n");
      return 1;
  }

  if (lUsage) {
    showUsage(argv[0]);
    return 1;
  }

 // add signal handler
  struct sigaction sa;
  sa.sa_handler = sigHandler;
  sa.sa_flags = SA_RESETHAND;

  sigaction(SIGINT,&sa,NULL);

  std::vector<Pds::Jungfrau::Module*> modules(num_modules);
  for (unsigned i=0; i<num_modules; i++) {
    modules[i] = new Pds::Jungfrau::Module(i, sSlsHost[i], sHost[i], port, sMac[i], sDetIp[i], use_flow_ctrl, configReceiver);
    printf("Module %u info:\n", i);
    printf(" - Module id:        %#lx\n", modules[i]->moduleid());
    printf(" - Serial number:    %#lx\n", modules[i]->serialnum());
    printf(" - Firmware version: %#lx\n", modules[i]->firmware());
    printf(" - Software version: %#lx\n", modules[i]->software());
  }
  Pds::Jungfrau::Detector* det = new Pds::Jungfrau::Detector(modules, threaded);
 
  Pds::Jungfrau::DacsConfig dacs_config(
    1000, // vb_ds
    1220, // vb_comp
    750,  // vb_pixbuf
    480,  // vref_ds
    420,  // vref_comp
    1450, // vref_prech
    1053, // vin_com
    3000  // vdd_prot
  );
 
  if (!det->configure(external ? 0 : numImages, gain, speed, triggerDelay, exposureTime, exposurePeriod, bias, dacs_config)) {
    printf("failed to configure the detector!\n");
    if (det && configReceiver) {
      delete det;
      det = 0;
    }
    return 1;
  }

  sleep(1);

  size_t header_sz = sizeof(uint64_t)/sizeof(uint16_t);
  size_t event_sz = det->get_num_pixels() + header_sz;
  size_t  data_sz = numImages * event_sz;
  uint16_t* data = new uint16_t[data_sz];
  uint64_t frame = 0;
  JungfrauModInfoType* metadata = new JungfrauModInfoType[num_modules];
  uint64_t last = 0;

  det->sync_next_frame();

  if (det->start()) {
    for(int i=0; i<numImages; i++) {
      if (!running) {
        break;
      }

      if (det->get_frame(&frame, metadata, &data[i*event_sz + header_sz])) {
        if (show_info) {
          printf("got frame: %lu (ts %g, delta %g)\n",
                 frame,
                 metadata->timestamp()/tsClock,
                 (metadata->timestamp()-last)/tsClock);
          last = metadata->timestamp();
        } else {
          printf("got frame: %lu\n",
                 frame);
        }
        *((uint64_t*) &data[i*event_sz]) = frame;
      } else {
        printf("failed to retrieve frame: %lu\n", frame);
      }
    }
    det->stop();
    sleep(1);
  } else {
    printf("failed to start detector!\n");
  }

  if (fileName && running) {
    printf("Writing %d Jungfrau frames to %s\n", numImages, fileName);
    FILE *f = fopen(fileName, "wb");
    fwrite(data, sizeof(uint16_t), data_sz, f);
    fclose(f);
  }

  if (det && configReceiver) {
    delete det;
    det = 0;
  }
  if (data) {
    delete[] data;
  }
  if (fileName) {
    delete[] fileName;
  }

  return 0;
}
