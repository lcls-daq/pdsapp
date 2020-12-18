#include "pds/zyla/Driver.hh"
#include "pds/service/CmdLineTools.hh"

#include <getopt.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>

static AT_H Handle = AT_HANDLE_UNINITIALISED;
static const int MAX_CONN_RETRY = 10;

static void close_camera(int isig)
{
  AT_Close(Handle);
  AT_FinaliseLibrary();
  exit(0);
}

static void show_usage(const char* p)
{
  printf("Usage: %s [OPTIONS]\n"
         "\n"
         "    -c|--camera   [0-9]                     select the Andor SDK camera index. (default: 0)\n"
         "    -n|--number   <number of images>        number of images to be captured (default: 1)\n"
         "    -w|--write    <filename prefix>         output filename prefix\n"
         "    -s|--shutter  <shutter mode>            0: Rolling, 1: Global (default: Global)\n"
         "    -r|--roi      <x,y,w,h>                 the dimensions of the camera roi (default: 1,1,2560,2160)\n"
         "    -b|--binning  <x,y>                     the pixel binning of the camera (default: 1,1)\n"
         "    -e|--exposure <exposure>                the exposure time for images in seconds (default: 0.001)\n"
         "    -C|--cooling                            enable cooling of the camera\n"
         "    -o|--overlap                            use overlap mode for camera readout\n"
         "    -t|--trigger                            use an external trigger instead for acquistion\n"
         "    -h|--help                               print this message and exit\n", p);
}

using namespace Pds::Zyla;

int main(int argc, char **argv)
{
  const char*         strOptions  = ":hc:n:w:s:r:b:e:Cot";
  const struct option loOptions[] = {
    {"help",     0, 0, 'h'},
    {"camera",   1, 0, 'c'},
    {"nframes",  1, 0, 'n'},
    {"write",    1, 0, 'w'},
    {"shutter",  1, 0, 's'},
    {"roi",      1, 0, 'r'},
    {"binning",  1, 0, 'b'},
    {"exposure", 1, 0, 'e'},
    {"cooling",  0, 0, 'C'},
    {"overlap",  0, 0, 'o'},
    {"trigger",  0, 0, 't'},
    {0,          0, 0,  0 }
  };

  bool lUsage = false;
  int camera_index = 0;
  int num_frames = 1;
  bool enable_cooling = false;
  bool overlap = false;
  AT_64 width = 2560;
  AT_64 height = 2160;
  AT_64 orgX = 1;
  AT_64 orgY = 1;
  AT_64 binX = 1;
  AT_64 binY = 1;
  double exposure = 0.001;
  bool noise_filter = false;
  bool blemish_correction = false;
  Driver::CoolingSetpoint cooling_spt = Driver::Temp_0C;
  Driver::FanSpeed fan_speed = Driver::On;
  Driver::ShutteringMode shutter = Driver::Global;
  Driver::ReadoutRate readout_rate = Driver::Rate280MHz;
  Driver::GainMode gain = Driver::LowNoiseHighWellCap16Bit;
  Driver::TriggerMode trigger = Driver::Internal;
  char* file_prefix = (char *)NULL;
  
  int optIndex = 0;
  while (int opt = getopt_long(argc, argv, strOptions, loOptions, &optIndex)) {
    if (opt == -1) break;

    switch(opt) {
      case 'h':
        show_usage(argv[0]);
        return 0;
      case 'c':
        if (!Pds::CmdLineTools::parseInt(optarg,camera_index)) {
          printf("%s: option `-c' parsing error\n", argv[0]);
          lUsage = true;
        }
        break;
      case 'n':
        if (!Pds::CmdLineTools::parseInt(optarg,num_frames)) {
          printf("%s: option `-n' parsing error\n", argv[0]);
          lUsage = true;
        }
        break;
      case 'w':
        file_prefix = new char[strlen(optarg)+1];
        strcpy(file_prefix, optarg);
        break;
      case 'r':
        {
          char* next_token = optarg;
          orgX = strtoul(next_token, &next_token, 0); ++next_token;
          if ( *next_token == 0 ) break;
          orgY = strtoul(next_token, &next_token, 0); ++next_token;
          if ( *next_token == 0 ) break;
          width = strtoul(next_token, &next_token, 0); ++next_token;
          if ( *next_token == 0 ) break;
          height = strtoul(next_token, &next_token, 0); ++next_token;
        }
        break;
      case 'b':
        {
          char* next_token = optarg;
          binX = strtoul(next_token, &next_token, 0); ++next_token;
          if ( *next_token == 0 ) break;
          binY = strtoul(next_token, &next_token, 0); ++next_token;
        }
        break;
      case 'e':
        if (!Pds::CmdLineTools::parseDouble(optarg,exposure)) {
          printf("%s: option `-e' parsing error\n", argv[0]);
          lUsage = true;
        }
        break;
      case 'C':
        enable_cooling = true;
        break;
      case 'o':
        overlap = true;
        break;
      case 't':
        trigger = Driver::External;
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
        lUsage = true;
        break;
      default:
        lUsage = true;
        break;
    }
  }

  if (optind < argc) {
    printf("%s: invalid argument -- %s\n", argv[0], argv[optind]);
    lUsage = true;
  }

  if (lUsage) {
    show_usage(argv[0]);
    return 1;
  }

  if (AT_InitialiseLibrary() != AT_SUCCESS) {
    printf("Failed to initialize Andor SDK! - exitting...\n");
    return 1;
  }

  if (AT_InitialiseUtilityLibrary() != AT_SUCCESS) {
    printf("Failed to initialize Andor SDK Utilities! - exitting...\n");
    return 1;
  }

  // Get the number of devices
  AT_64 dev_count;
  if (AT_GetInt(AT_HANDLE_SYSTEM, L"Device Count", &dev_count) != AT_SUCCESS) {
    printf("Failed to retrieve device count from SDK! - exitting...\n");
    return 1;
  }

  if (camera_index > (dev_count-1)) {
    printf("Requested camera index (%d) is out of range of the %lld available cameras!\n", camera_index, dev_count);
    return 1;
  }
  
  //AT_H Handle;
  AT_Open(camera_index, &Handle);

  /*
   * Register singal handler
   */
  struct sigaction sigActionSettings;
  sigemptyset(&sigActionSettings.sa_mask);
  sigActionSettings.sa_handler = close_camera;
  sigActionSettings.sa_flags   = SA_RESTART;

  if (sigaction(SIGINT, &sigActionSettings, 0) != 0 )
    printf("Cannot register signal handler for SIGINT\n");
  if (sigaction(SIGTERM, &sigActionSettings, 0) != 0 )
    printf("Cannot register signal handler for SIGTERM\n");

  Driver* camera = new Driver(Handle);
  bool cam_timeout = false;
  int retry_count = 0;
  printf("Waiting to camera to initialize ...");
  while(!camera->is_present()) {
    if (retry_count > MAX_CONN_RETRY) break;
    printf(".");
    sleep(1);
    retry_count++;
  }
  if (cam_timeout) {
    printf(" timeout!\n");
    printf("Camera is not present!\n");
    return 1;
  } else {
    printf(" done!\n");
  }

  AT_WC buffer[256];
  camera->get_model(buffer, 256);
  printf("Camera model: %ls\n", buffer);
  camera->get_name(buffer, 256);
  printf("Camera name: %ls\n", buffer);
  camera->get_family(buffer, 256);
  printf("Camera family: %ls\n", buffer);
  camera->get_serial(buffer, 256);
  printf("Camera serial: %ls\n", buffer);

  char fname[128];

  camera->set_image(width, height, orgX, orgY, binX, binY, noise_filter, blemish_correction);
  camera->set_readout(shutter, readout_rate, gain);
  camera->set_trigger(trigger, 0.0, overlap);
  camera->set_exposure(exposure);
  camera->set_cooling(enable_cooling, cooling_spt, fan_speed);
  camera->configure(num_frames);

  printf("Camera configure for an image of %lld w by %lld h\n", camera->image_width(), camera->image_height());

  camera->get_cooling_status(buffer, 256);
  printf("Camera cooling status: %ls\n", buffer);
  printf("Camera temperature: %G C\n", camera->temperature());
  
  size_t frame_size = camera->frame_size();
  uint16_t* data = new uint16_t[frame_size];
  AT_64 frame_timestamp = 0;

  camera->wait_cooling(0);

  if (trigger == Driver::External) {
    printf("Using external trigger for acquistion\n");
  } else {
    printf("Estimated camera frame rate (Hz) : %g\n", camera->frame_rate());
  }
  printf("Image exposure time (sec) : %g\n", camera->exposure());
  printf("Estimated camera readout time (sec) : %g\n", camera->readout_time());
  printf("Capturing %d images from the camera\n", num_frames);
  if (file_prefix)
    printf("Images will be saved with the naming scheme: %sXXX.raw\n", file_prefix);
  camera->start();
  for (int i=0; i<num_frames; i++) {
    if (camera->get_frame(frame_timestamp, data)) {
      if (file_prefix) {
        sprintf(fname, "%s%03d.raw", file_prefix, i+1);
        FILE *img = fopen(fname, "wb");
        fwrite(data, sizeof(uint16_t), frame_size, img);
        fclose(img);
      }
      printf("Completed image %d of %d (timestamp: %llu)\n", i+1, num_frames, frame_timestamp);
    } else {
      printf("Failed to capture image %d of %d\n", i+1, num_frames);
    }
  }
  camera->stop();
  camera->close();

  if (file_prefix) delete[] file_prefix;
  delete[] data;

  AT_FinaliseUtilityLibrary();
  AT_FinaliseLibrary();

  return 0;
}
