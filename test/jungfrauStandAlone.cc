#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <getopt.h>
#include "pds/jungfrau/Driver.hh"

#define PIXELS_PER_PACKET 4096
#define PACKETS_PER_FRAME 128

static const char sJungfrauTestVersion[] = "1.0";

static void showVersion(const char* p)
{
  printf( "Version:  %s  Ver %s\n", p, sJungfrauTestVersion );
}

static void showUsage(const char* p)
{
  printf("Usage: %s [-v|--version] [-h|--help] [-c|--camera <camera number>]\n"
         "[-w|--write <filename prefix>] [-n|--number <number of images>] [-e|--exposure <exposure time (sec)>]\n"
         "[-b|--bias <bias>] [-g|--gain <gain>] [-S|--speed <speed>] [-t|--trigger <delay>] [-r|--receiver]\n"
         "-H|--host <host> [-P|--port <port>] -m|--mac <mac> -d|--detip <detip> -s|--sls <sls>\n"
         " Options:\n"
         "    -c|--camera   [0-9]                     select the slsDetector device id. (default: 0)\n"
         "    -w|--write    <filename prefix>         output filename prefix\n"
         "    -n|--number   <number of images>        number of images to be captured (default: 1)\n"
         "    -e|--exposure <exposure time>           exposure time (sec) (default: 0.00001 sec)\n"
         "    -b|--bias     <bias>                    the bias voltage to apply to the sensor in volts (default: 200)\n"
         "    -g|--gain     <gain 0-5>                the gain mode of the detector (default: Normal - 0)\n"
         "    -S|--speed    <speed 0-1>               the clock speed mode of the detector (default: Quarter - 0)\n"
         "    -H|--host     <host>                    set the receiver host ip\n"
         "    -P|--port     <port>                    set the receiver udp port number (default: 32410)\n"
         "    -m|--mac      <mac>                     set the receiver mac address\n"
         "    -d|--detip    <detip>                   set the detector ip address\n"
         "    -s|--sls      <sls>                     set the hostname of the slsDetector interface\n"
         "    -t|--trigger  <delay>                   the internal acquisition start delay to use with an external trigger is seconds (default: 0.000238)\n"
         "    -r|--receiver                           attempt to configure ip settings of the receiver (default: false)\n"
         "    -v|--version                            show file version\n"
         "    -h|--help                               print this message and exit\n", p);
}

int main(int argc, char **argv)
{
  const char*         strOptions  = ":vhc:w:n:e:b:g:S:H:P:m:d:s:t:r";
  const struct option loOptions[] =
  {
    {"ver",         0, 0, 'v'},
    {"help",        0, 0, 'h'},
    {"camera",      1, 0, 'c'},
    {"write",       1, 0, 'w'},
    {"number",      1, 0, 'n'},
    {"exposure",    1, 0, 'e'},
    {"bias",        1, 0, 'b'},
    {"gain",        1, 0, 'g'},
    {"speed",       1, 0, 'S'},
    {"host",        1, 0, 'H'},
    {"port",        1, 0, 'P'},
    {"mac",         1, 0, 'm'},
    {"detip",       1, 0, 'd'},
    {"sls",         1, 0, 's'},
    {"trigger",     1, 0, 't'},
    {"receiver",    0, 0, 'r'},
    {0,             0, 0,  0 }
  };

  unsigned port  = 32410;
  int camera = 0;
  int numImages = 1;
  unsigned bias = 200;
  double exposureTime = 0.00001;
  double triggerDelay = 0.000238;
  bool lUsage = false;
  bool configReceiver = false;
  unsigned gain_value = 0;
  unsigned speed_value = 0;
  JungfrauConfigType::GainMode gain = JungfrauConfigType::Normal;
  JungfrauConfigType::SpeedMode speed = JungfrauConfigType::Quarter;
  char* sHost    = (char *)NULL;
  char* sMac     = (char *)NULL;
  char* sDetIp   = (char *)NULL;
  char* sSlsHost = (char *)NULL;
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
      case 'c':
        camera = strtol(optarg, NULL, 0);
        break;
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
        sHost = optarg;
        break;
      case 'P':
        port = strtoul(optarg, NULL, 0);
        break;
      case 'm':
        sMac = optarg;
        break;
      case 'd':
        sDetIp = optarg;
        break;
      case 's':
        sSlsHost = optarg;
        break;
      case 'r':
        configReceiver = true;
        break;
      case 'D':
        triggerDelay = strtod(optarg, NULL);
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

  if(!sHost) {
    printf("%s: receiver hostname is required\n", argv[0]);
    lUsage = true;
  }

  if(!sMac) {
    printf("%s: receiver mac address is required\n", argv[0]);
    lUsage = true;
  }

  if(!sDetIp) {
    printf("%s: detector ip address is required\n", argv[0]);
    lUsage = true;
  }

  if(!sSlsHost) {
    printf("%s: slsDetector interface hostname is required\n", argv[0]);
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

  Pds::Jungfrau::Driver* det = new Pds::Jungfrau::Driver(camera, sSlsHost, sHost, port, sMac, sDetIp, configReceiver);
  
  det->configure(numImages, gain, speed, triggerDelay, exposureTime, bias);

  sleep(1);

  size_t header_sz = sizeof(uint32_t)/sizeof(uint16_t);
  size_t event_sz = PIXELS_PER_PACKET * PACKETS_PER_FRAME + header_sz;
  size_t  data_sz = numImages * event_sz;
  uint16_t* data = new uint16_t[data_sz];
  int16_t frame = -1;

  if (det->start()) {
    for(int i=0; i<numImages; i++) {
      frame = det->get_frame(&data[i*event_sz + header_sz]);
      printf("got frame: %d\n", frame);
      *((uint32_t*) &data[i*event_sz]) = frame;
    }
    det->stop();
    sleep(1);
  }

  if (fileName) {
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

#undef PIXELS_PER_PACKET
#undef PACKETS_PER_FRAME
