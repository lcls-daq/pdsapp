#include "pds/vimba/Driver.hh"
#include "pds/vimba/FrameBuffer.hh"
#include "pds/vimba/Errors.hh"
#include "pds/service/CmdLineTools.hh"

#include <getopt.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>

static Pds::Vimba::SimpleFrameBuffer* framebuf = NULL;
static const char* GENICAM_ENV = "GENICAM_GENTL64_PATH";

static void close_camera(int isig)
{
  if (framebuf) {
    printf("Frame capture has been cancelled - cleaning up...\n");
    framebuf->cancel();
  }
}

static void show_usage(const char* p)
{
  printf("Usage: %s [OPTIONS]\n"
         "\n"
         "    -c|--camera   [0-9]                     select the Vimba SDK index. (default: 0)\n"
         "    -s|--serialid <serialid>                find camera via serialid otherwise index is used\n"
         "    -n|--number   <number of images>        number of images to be captured (default: 1)\n"
         "    -w|--write    <filename prefix>         output filename prefix\n"
         "    -r|--roi      <x,y,w,h>                 the dimensions of the camera roi\n"
         "    -e|--exposure <exposure time>           sets the exposure time of camera (default: 200us)\n"
         "    -p|--pixel    <pixel mode>              sets the pixel format\n"
         "                  0=Mono8 (default)\n"
         "                  1=Mono10\n"
         "                  2=Mono10p\n"
         "                  3=Mono12\n"
         "                  4=Mono12p\n"
         "                  5=Mono14\n"
         "                  6=Mono16\n"
         "    -f|--format                             convert pixel format of images to 16bits per pixel\n"
         "    -l|--list                               list the features of the camera\n"
         "    -L|--limit    <link limit>              limit the link speed to this value (default: 450000000 Bytes/s)\n"
         "    -S|--stats                              display stats about captured frames\n"
         "    -t|--trigger                            use external trigger\n"
         "    -m|--maxfails <max fails>               maximum consecutive frame capture fails before giving up (default: 5)\n"
         "    -h|--help                               print this message and exit\n", p);
}

using namespace Pds::Vimba;

int main(int argc, char **argv)
{
  const char*         strOptions  = ":hc:s:n:w:r:e:p:flL:Stm:";
  const struct option loOptions[] = {
    {"help",     0, 0, 'h'},
    {"camera",   1, 0, 'c'},
    {"serialid", 1, 0, 's'},
    {"nframes",  1, 0, 'n'},
    {"write",    1, 0, 'w'},
    {"roi",      1, 0, 'r'},
    {"exposure", 1, 0, 'e'},
    {"pixel",    1, 0, 'p'},
    {"format",   1, 0, 'f'},
    {"list",     0, 0, 'l'},
    {"limit",    1, 0, 'L'},
    {"stats",    0, 0, 'S'},
    {"trigger",  0, 0, 't'},
    {"maxfails", 1, 0, 'm'},
    {0,          0, 0,  0 }
  };

  bool lUsage = false;
  bool list_features = false;
  bool show_stats = false;
  bool use_roi = false;
  bool reformat_pixels = false;
  int camera_index = 0;
  unsigned num_frames = 1;
  unsigned num_buffers = 64;
  unsigned link_limit = 450000000;
  unsigned raw_pixel_format = 0;
  unsigned offset_x = 0, offset_y = 0, height = -1, width = -1;
  unsigned max_fails = 5;
  const char* file_prefix = NULL;
  double exposure = 200.0;
  Camera::TriggerMode trig_mode = Camera::FreeRun;
  const char* serial_id = NULL;
  VmbError_t err = VmbErrorSuccess;
  VmbCameraInfo_t info;
  VmbVersionInfo_t version;
  VmbTransportLayerInfo_t tl_info;
  Camera* cam = NULL;
  Camera::PixelFormat pixel_format = Camera::Mono8;

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
      case 's':
        serial_id = optarg;
        break;
      case 'n':
        if (!Pds::CmdLineTools::parseUInt(optarg,num_frames)) {
          printf("%s: option `-n' parsing error\n", argv[0]);
          lUsage = true;
        }
        break;
      case 'w':
        file_prefix = optarg;
        break;
      case 'r':
        if (Pds::CmdLineTools::parseUInt(optarg,offset_x,offset_y,width,height)!=4) {
          printf("%s: option `-r' parsing error\n", argv[0]);
          lUsage = true;
        } else {
          use_roi = true;
        }
        break;
      case 'e':
        if (!Pds::CmdLineTools::parseDouble(optarg,exposure)) {
          printf("%s: option `-e' parsing error\n", argv[0]);
          lUsage = true;
        }
        break;
      case 'p':
        if (!Pds::CmdLineTools::parseUInt(optarg,raw_pixel_format)) {
          printf("%s: option `-p' parsing error\n", argv[0]);
          lUsage = true;
        } else if (raw_pixel_format < Camera::UnsupportedFormat) {
          pixel_format = static_cast<Camera::PixelFormat>(raw_pixel_format);  
        } else {
          printf("%s: option `-p' value out of range\n", argv[0]);
          lUsage = true;
        }
        break;
      case 'f':
        reformat_pixels = true;
        break;
      case 'l':
        list_features = true;
        break;
      case 'L':
        if (!Pds::CmdLineTools::parseUInt(optarg,link_limit)) {
          printf("%s: option `-L' parsing error\n", argv[0]);
          lUsage = true;
        }
        break;
      case 'S':
        show_stats = true;
        break;
      case 't':
        trig_mode = Camera::External;
        break;
      case 'm':
        if (!Pds::CmdLineTools::parseUInt(optarg,max_fails)) {
          printf("%s: option `-m' parsing error\n", argv[0]);
          lUsage = true;
        }
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

  // if the genicam path is not found set the default one
  setenv(GENICAM_ENV, Camera::getGeniCamPath().c_str(), 0);
  // show the genicam path
  printf("Using %s=\"%s\"\n", GENICAM_ENV, getenv(GENICAM_ENV));

  // initialize the vimba sdk
  if ((err = VmbStartup(NULL)) != VmbErrorSuccess) {
    printf("Failed to initialize Vimba SDK!: %s\n", ErrorCodes::desc(err));
    return 1;   
  }

  // get vimba version info
  if (Camera::getVersionInfo(&version)) {
    printf("Using Vimba SDK version %u.%u.%u\n", version.major, version.minor, version.patch);
  } else {
    printf("Failed to query Vimba SDK version! - exitting...\n");
    VmbShutdown();
    return 1;
  }

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

  // Retrieve camera info
  if (Camera::getCameraInfo(&info, camera_index, serial_id)) {
    // list info on the camera
    printf("Found camera:\n"
           "  Name       - %s\n"
           "  Model      - %s\n"
           "  ID         - %s\n"
           "  ExtendedId - %s\n"
           "  Serial Num - %s\n",
           info.cameraName,
           info.modelName,
           info.cameraIdString,
           info.cameraIdExtended,
           info.serialString);
    if (Camera::getTransportLayerInfo(&tl_info, info.transportLayerHandle)) {
      printf("Camera Transport Layer:\n");
      printf("  ID         - %s\n", tl_info.transportLayerIdString);
      printf("  Name       - %s\n", tl_info.transportLayerName);
      printf("  Model      - %s\n", tl_info.transportLayerModelName);
      printf("  Vendor     - %s\n", tl_info.transportLayerVendor);
      printf("  Version    - %s\n", tl_info.transportLayerVersion);
      printf("  Path       - %s\n", tl_info.transportLayerPath);
      printf("  Type       - %s\n", TransportTypes::desc(tl_info.transportLayerType));
    }
    // Initialize the camera object
    cam = new Camera(&info);
    if (cam->isOpen()) {
      if (list_features) {
        // List the camera features
        cam->listFeatures();
      } else {
        try {
          bool configured = true;
          // set link speed
          configured = configured && cam->setDeviceLinkThroughputLimit(link_limit);
          // set number of frames
          configured = configured && cam->setAcquisitionMode(num_frames);
          // set trigger
          configured = configured && cam->setTriggerMode(trig_mode, exposure);
          // set roi if requested
          if (use_roi) {
            configured = configured && cam->setImageRoi(offset_x, offset_y, width, height);
          } else {
            // camera remembers old roi so make sure to clear
            configured = configured && cam->unsetImageRoi();
          }
          // set pixel format
          configured = configured && cam->setPixelFormat(pixel_format);
          // check that configuration was successful
          if (configured) {
            // Create the frame buffer / acquisition handler
            framebuf = new SimpleFrameBuffer(num_buffers, num_frames, cam, file_prefix, show_stats, reformat_pixels);

            // show image format information
            printf("Image information:\n");
            printf("  Width (Max):            %lld (%lld)\n", cam->width(), cam->widthMax());
            printf("  Height (Max):           %lld (%lld)\n", cam->height(), cam->heightMax());
            printf("  Offset X:               %lld\n", cam->offsetX());
            printf("  Offset Y:               %lld\n", cam->offsetY());
            printf("  Reverse X:              %s\n", Bool::desc(cam->reverseX()));
            printf("  Reverse Y:              %s\n", Bool::desc(cam->reverseY()));
            printf("  Sensor Width:           %lld\n", cam->sensorWidth());
            printf("  Sensor Height:          %lld\n", cam->sensorHeight());
            printf("  Shutter Mode:           %s\n", cam->shutterMode());
            printf("  Pixel Format:           %s\n", cam->pixelFormat());
            printf("  Pixel Size:             %s\n", cam->pixelSize());
            // show trigger information
            printf("Trigger information:\n");
            printf("  Trigger Mode:           %s\n", cam->triggerMode());
            printf("  Exposure Mode:          %s\n", cam->exposureMode());
            printf("  Exposure Time (us):     %f\n", cam->exposureTime());
            // show contrast information
            printf("Contrast information:\n");
            printf("  Contrast Enable:        %s\n", Bool::desc(cam->contrastEnable()));
            printf("  Contrast Shape:         %lld\n", cam->contrastShape());
            printf("  Contrast Dark Limit:    %lld\n", cam->contrastDarkLimit());
            printf("  Contrast Bright Limit:  %lld\n", cam->contrastBrightLimit());
            // show analog control information
            printf("Analog control information:\n");
            printf("  Black Level:            %f\n", cam->blackLevel());
            printf("  Black Selector:         %s\n", cam->blackLevelSelector());
            printf("  Gain:                   %f\n", cam->gain());
            printf("  Gain Selector:          %s\n", cam->gainSelector());
            printf("  Gamma:                  %f\n", cam->gamma());
            // show image correction information
            if (cam->imageCorrectionAvailable()) {
              printf("Correction information:\n");
              printf("  Correction Mode:        %s\n", cam->correctionMode());
              printf("  Correction Selector:    %s\n", cam->correctionSelector());
              printf("  Correction Set:         %s\n", cam->correctionSet());
              printf("  Correction Set Default: %s\n", cam->correctionSetDefault());
              printf("  Correction Data Size:   %lld\n", cam->correctionDataSize());
              printf("  Correction Entry Type:  %lld\n", cam->correctionEntryType());
            }
            // show device information
            printf("Device information:\n");
            printf("  Vendor name:            %s\n", cam->deviceVendorName().c_str());
            printf("  Family name:            %s\n", cam->deviceFamilyName().c_str());
            printf("  Model name:             %s\n", cam->deviceModelName().c_str());
            printf("  Manufacturer ID:        %s\n", cam->deviceManufacturer().c_str());
            printf("  Version:                %s\n", cam->deviceVersion().c_str());
            printf("  Serial number           %s\n", cam->deviceSerialNumber().c_str());
            printf("  Firmware ID:            %s\n", cam->deviceFirmwareID().c_str());
            printf("  Firmware Version:       %s\n", cam->deviceFirmwareVersion().c_str());
            printf("  User ID:                %s\n", cam->deviceUserID().c_str());
            printf("  GenCP Version:          %lld.%lld\n", cam->deviceGenCPMajorVersion(), cam->deviceGenCPMinorVersion());
            printf("  TL Version:             %lld.%lld\n", cam->deviceTLMajorVersion(), cam->deviceTLMinorVersion());
            printf("  SFNC Version:           %lld.%lld.%lld\n",
                   cam->deviceSFNCMajorVersion(), cam->deviceSFNCMinorVersion(), cam->deviceSFNCPatchVersion());
            printf("  Link speed (B/s):       %lld\n", cam->deviceLinkSpeed());
            printf("  Link timeout (us):      %f\n", cam->deviceLinkCommandTimeout());
            printf("  Link limit mode:        %s\n", cam->deviceLinkThroughputLimitMode());
            printf("  Link limit (B/s):       %lld\n", cam->deviceLinkThroughputLimit());
            printf("  Scan type:              %s\n", cam->deviceScanType());
            printf("  LED mode:               %s\n", cam->deviceIndicatorMode());
            printf("  LED brightness:         %lld\n", cam->deviceIndicatorLuminance());
            printf("  Temperature (C):        %f (%s)\n", cam->deviceTemperature(), cam->deviceTemperatureSelector());

            // allocate frames, start capture engine, and queue frames
            framebuf->configure();

            // show readout buffer information
            printf("Buffer information:\n");
            printf("  Driver Buffer Count:    %lld\n", cam->maxDriverBuffersCount());
            printf("  Stream Buffer Mode:     %s\n", cam->streamBufferHandlingMode());
            printf("  Stream Buffer Minimum:  %lld\n", cam->streamAnnounceBufferMinimum());
            printf("  Stream Buffer Count:    %lld\n", cam->streamAnnouncedBufferCount());
            printf("  Stream ID:              %s\n", cam->streamID().c_str());
            printf("  Stream Type:            %s\n", cam->streamType());
            printf("  Stream Running:         %s\n", Bool::desc(cam->streamIsGrabbing()));
            // show the maximum frame rate based on current camera settings
            printf("Maximum frame rate is %f Hz\n", cam->acquisitionFrameRate());
            if (file_prefix) {
              printf("Images will be saved with the naming scheme: %sXXX.raw\n", file_prefix);
            }

            // only start acquisition if we intended to acquire frames
            if (num_frames > 0) {
              // start acquisition
              framebuf->enable();
              // wait for acquisition to finish or be canceled
              framebuf->wait();
              // stop acquistion
              framebuf->disable();
            }

            // flush queue and deallocate frames
            framebuf->unconfigure();
          }
        } catch(const VimbaException& e) {
          printf("Exception encountered configuring camera: %s\n", e.what());
        }
      }
    } else {
      printf("Failed to open camera (%s)!\n", info.cameraIdString);
    }
  } else {
    printf("Failed to find a camera\n");
  }

  // delete the frame buffer object
  if (framebuf) {
    delete framebuf;
    framebuf = NULL;
  }
  // delete the camera object
  if (cam) {
    delete cam;
    cam = NULL;
  }

  // shutdown the vimba sdk
  VmbShutdown();

  return 0;
}
