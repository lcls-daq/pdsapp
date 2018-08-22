#include <getopt.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>

#include "pds/service/CmdLineTools.hh"
#include "pds/picam/PiUtils.hh"

#define PRINT_PICAM_ERROR(errorCode, strScope) \
if (!piIsFuncOk(errorCode)) \
{ \
  if (errorCode == PicamError_ParameterDoesNotExist) \
    printf("%s: %s\n", strScope, piErrorDesc(iError)); \
  else \
  { \
    printf("%s failed: %s\n", strScope, piErrorDesc(iError)); \
  } \
}

#define CHECK_PARAM_RANGE(name, param, constraint) \
if (param < (int) constraint.minimum) { \
  printf("%s: value %d is less than min %d\n", name, param, (int) constraint.minimum); \
  param = constraint.minimum; \
} else if (param > (int) constraint.maximum) {  \
  printf("%s: value %d is greater than max %d\n", name, param, (int) constraint.maximum); \
  param = constraint.maximum; \
}

const static int MaxCoolingTime = 100;
static PicamHandle camera = NULL;
static PicamCameraID id;

static void close_camera(int isig)
{
  if (camera) {
    Picam_StopAcquisition(camera);
    Picam_CloseCamera(camera);
    camera = NULL;
    Picam_UninitializeLibrary();
  }
  exit(0);
}

static void show_usage(const char* p)
{
  printf("Usage: %s [OPTIONS]\n"
         "\n"
         "    -c|--camera   [0-9]                     select the Picam SDK camera index. (default: 0)\n"
         "    -n|--number   <number of images>        number of images to be captured (default: 1)\n"
         "    -w|--write    <filename prefix>         output filename prefix\n"
         "    -r|--roi      <x,y,w,h>                 the dimensions of the camera roi (default: 1,1,2048,2048)\n"
         "    -b|--binning  <x,y>                     the pixel binning of the camera (default: 1,1)\n"
         "    -e|--exposure <exposure>                the exposure time for images in milliseconds (default: 100)\n"
         "    -C|--cooling  <temp>                    enable cooling of the camera\n"
         "    -t|--trigger                            use an external trigger instead for acquistion\n"
         "    -l|--list                               list all parameters of the camera\n"
         "    -h|--help                               print this message and exit\n", p);
}

using namespace PiUtils;

int main(int argc, char **argv)
{
  const char*         strOptions  = ":hc:n:w:r:b:e:C:tl";
  const struct option loOptions[] = {
    {"help",     0, 0, 'h'},
    {"camera",   1, 0, 'c'},
    {"nframes",  1, 0, 'n'},
    {"write",    1, 0, 'w'},
    {"roi",      1, 0, 'r'},
    {"binning",  1, 0, 'b'},
    {"exposure", 1, 0, 'e'},
    {"cooling",  1, 0, 'C'},
    {"trigger",  0, 0, 't'},
    {"list",     0, 0, 'l'},
    {0,          0, 0,  0 }
  };

  int iError = PicamError_None;
  bool lUsage = false;
  int camera_index = 0;
  int num_frames = 1;
  int timeout = -1;
  bool enable_cooling = false;
  bool external_trigger = false;
  bool requested_roi = false;
  bool list_parameters = false;
  piint width = 2048;
  piint height = 2048;
  piint orgX = 0;
  piint orgY = 0;
  piint binX = 1;
  piint binY = 1;
  double exposure = 100.0;
  double cooling_setpoint = 25.0;
  char* file_prefix = (char *)NULL;
  char fname[128];
  float fTemperature = 999;
  std::string sTemperatureStatus;

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
          requested_roi = true;
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
        if (!Pds::CmdLineTools::parseDouble(optarg,cooling_setpoint)) {
          printf("%s: option `-C' parsing error\n", argv[0]);
          lUsage = true;
        }
        break;
      case 't':
        external_trigger = true;
        break;
      case 'l':
        list_parameters = true;
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

  if (!piIsFuncOk(Picam_InitializeLibrary())) {
    printf("Failed to initialize Picam SDK! - exitting...\n");
    return 1;
  }

  int numCamera;
  const PicamCameraID *listCamID;
  const pichar* string;
  PicamAvailableData data;
  PicamAcquisitionErrorsMask errors;
  piint readoutstride = 0;

  Picam_GetAvailableCameraIDs(&listCamID, &numCamera);

  for (int iCamera=0; iCamera<numCamera; ++iCamera) {
    const PicamFirmwareDetail*  firmware_array;
    piint                       firmware_count;
    if (piIsFuncOk(Picam_GetFirmwareDetails(&listCamID[iCamera], &firmware_array, &firmware_count))) {
      printf("  camera %d : %s\n", iCamera, piCameraDesc(listCamID[iCamera]));
      for (int iFirmware = 0; iFirmware < firmware_count; ++iFirmware)
        printf("    firmware [%d] %s : %s\n", iFirmware, firmware_array[iFirmware].name, firmware_array[iFirmware].detail);
      Picam_DestroyFirmwareDetails(firmware_array);
    }
  }

  // Check if the requested camera number exists
  if (camera_index >= numCamera) {
    printf("Requested camera index (%d) is out of range of the %d available cameras!\n", camera_index, numCamera);
    Picam_UninitializeLibrary();
    return 1;
  }

  // First check if a demo camera has been requested
  if (camera_index < 0) {
    printf("Requested Camera %d. Using a simulated camera\n", camera_index);
    Picam_ConnectDemoCamera(PicamModel_PixisXF2048B, "0008675309", &id);
  } else {
    id = listCamID[camera_index];
  }
  Picam_DestroyCameraIDs(listCamID);

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

  // Try to open the camera
  iError = Picam_OpenCamera(&id, &camera);
  if (!piIsFuncOk(iError)) {
    printf("Picam_OpenCamera() failed: %s\n", piErrorDesc(iError));
    camera = NULL;
    Picam_UninitializeLibrary();
    return 1;
  }

  // Print out some info on the camera
  Picam_GetEnumerationString( PicamEnumeratedType_Model, id.model, &string );
  printf("%s", string);
  printf(" (SN:%s) [%s]\n", id.serial_number, id.sensor_name);
  Picam_DestroyString( string );

  // Get detector dimensions
  const PicamRoisConstraint *pRoiConstraint;
  iError = Picam_GetParameterRoisConstraint(camera, PicamParameter_Rois, PicamConstraintCategory_Required, &pRoiConstraint);
  if (piIsFuncOk(iError)) {
    if (requested_roi) {
      CHECK_PARAM_RANGE("roi width", width, pRoiConstraint->width_constraint);
      CHECK_PARAM_RANGE("roi height", height, pRoiConstraint->height_constraint);
      CHECK_PARAM_RANGE("roi x", orgX, pRoiConstraint->x_constraint);
      CHECK_PARAM_RANGE("roi y", orgY, pRoiConstraint->y_constraint);
    } else {
      width = pRoiConstraint->width_constraint.maximum;
      height = pRoiConstraint->height_constraint.maximum;
      orgX = pRoiConstraint->x_constraint.minimum;
      orgY = pRoiConstraint->y_constraint.minimum;      
    }
  }

  // set the roi of the camera
  PicamRoi roi =
  {
    orgX,   // x
    width,  // width
    binX,   // x binning
    orgY,   // y
    height, // height
    binY,   // y binning
  };
  PicamRois rois = { &roi, 1 };
  iError = Picam_SetParameterRoisValue(camera, PicamParameter_Rois, &rois);
  PRINT_PICAM_ERROR(iError, "Picam_SetParameterRoisValue()");

  // set external trigger if requested
  if (external_trigger) {
    iError = Picam_SetParameterIntegerValue(
      camera,
      PicamParameter_TriggerSource,
      PicamTriggerSource_External
    );
    PRINT_PICAM_ERROR(iError, "Picam_SetParameterIntegerValue(PicamParameter_TriggerSource)");
  }

  if (enable_cooling) {
    if (piIsFuncOk(piReadTemperature(camera, fTemperature, sTemperatureStatus))) {
      printf("Temperature Before cooling: %g C  Status %s\n", fTemperature, sTemperatureStatus.c_str());
    }
    printf("Set Temperature to %g C\n", cooling_setpoint);
    iError = Picam_SetParameterFloatingPointValue(camera, PicamParameter_SensorTemperatureSetPoint, cooling_setpoint);
    PRINT_PICAM_ERROR(iError, "Picam_SetParameterFloatingPointValue(PicamParameter_SensorTemperatureSetPoint)");
  }

  iError = Picam_SetParameterFloatingPointValue(camera, PicamParameter_ExposureTime, exposure);
  PRINT_PICAM_ERROR(iError, "Picam_SetParameterFloatingPointValue(PicamParameter_ExposureTime)");

  // commit all the parameter changes
  iError = piCommitParameters(camera);
  if (iError != 0) printf("Failed to commit parameters to camera!\n");

  if (enable_cooling) {
    const static timeval timeSleepMicroOrg = {0, 5000}; // 5 millisecond
    timespec timeVal1;
    clock_gettime( CLOCK_REALTIME, &timeVal1 );

    int iNumLoop       = 0;
    int iNumRepateRead = 5;
    int iRead          = 0;

    while (true) {
      fTemperature = 999;
      iError = piReadTemperature(camera, fTemperature, sTemperatureStatus);

      if ( fTemperature <= cooling_setpoint ) {
        if ( ++iRead >= iNumRepateRead )
          break;
      }
      else
        iRead = 0;

      if ( (iNumLoop+1) % 200 == 0 )
        printf("Temperature *Updating*: %g C\n", fTemperature );

      timespec timeValCur;
      clock_gettime( CLOCK_REALTIME, &timeValCur );
      int iWaitTime = (timeValCur.tv_nsec - timeVal1.tv_nsec) / 1000000 +
      ( timeValCur.tv_sec - timeVal1.tv_sec ) * 1000; // in milliseconds
      if ( iWaitTime > MaxCoolingTime ) break;

      // This data will be modified by select(), so need to be reset
      timeval timeSleepMicro = timeSleepMicroOrg;
      // Use select() to simulate nanosleep(), because experimentally select() controls the sleeping time more precisely
      select( 0, NULL, NULL, NULL, &timeSleepMicro);
      iNumLoop++;
    }
  }

  Picam_GetParameterIntegerValue( camera, PicamParameter_ReadoutStride, &readoutstride );
  if (file_prefix)
    printf("Images will be saved with the naming scheme: %sXXX.raw\n", file_prefix);
  printf("Waiting for %d frames to be collected\n\n", num_frames);
  if(!piIsFuncOk(Picam_Acquire(camera, num_frames, timeout, &data, &errors))) {
    printf("Error: Camera only collected %d frames\n", (int) data.readout_count);
  } else {
    printf("Camera collected %d frames\n", (int) data.readout_count);
    if (file_prefix) {
      pibyte* buf = (pibyte*)data.initial_readout;
      pibyte* frameptr = NULL;
      for(piint loop = 0; loop < num_frames; loop++) {
        frameptr = buf + (readoutstride * loop);
        sprintf(fname, "%s%03d.raw", file_prefix, loop+1);
        FILE *img = fopen(fname, "wb");
        fwrite(frameptr, sizeof(pibyte), readoutstride, img);
        fclose(img);
      }
    }
  }

  if (list_parameters)
    piPrintAllParameters(camera);

  Picam_CloseCamera(camera);
  camera = NULL;
  
  Picam_UninitializeLibrary();

  return 0;
}

#undef PRINT_PICAM_ERROR
#undef CHECK_PARAM_RANGE
