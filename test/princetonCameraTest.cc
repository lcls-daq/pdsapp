#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <math.h>
#include <errno.h>
#include <getopt.h>
#include <string>
#include <iostream>
#include <pthread.h>
#include <unistd.h>
#include <signal.h>
#include "pds/princeton/PrincetonUtils.hh"
#include "pvcam/include/master.h"
#include "pvcam/include/pvcam.h"

int testPIDaq(int iCamera, char* sFnPrefix, int iNumImages, int iExposureTime, int iReadoutPort, int iSpeedIndex,
  int iGainIndex, float fTemperature, int iClearCycle, int iStrip, int iKinH, float fVsSpeed, int iCustW, int iCustH, int iTrgEdge,
  int iRoiX, int iRoiY, int iRoiW, int iRoiH, int iBinX, int iBinY, int iTrgMode, int iMenu);

static const char sPrincetonCameraTestVersion[] = "1.50";

using PICAM::printPvError;

class ImageCapture
{
public:

  int   start(int iCamera, char* sFnPrefix, int iNumImages, int iExposureTime, int iReadoutPort, int iSpeedIndex,
    int iGainIndex, float fTemperature, int iClearCycle, int iStrip, int iKinH, float fVsSpeed, int iCustW, int iCustH, int iTrgEdge,
    int iRoiX, int iRoiY, int iRoiW, int iRoiH, int iBinX, int iBinY, int iTrgMode, int iMenu);
  int   getConfig(int& x, int& y, int& iWidth, int& iHeight, int& iBinning);
  int   lockCurrentFrame(unsigned char*& pCurrentFrame);
  int   unlockFrame();

private:
  // private functions
  int testImageCaptureStandard(const char* sFnPrefix, int iNumFrame, int16 modeExposure, uns32 uExposureTime, int iRoiX, int iRoiY, int iRoiW, int iRoiH, int iBinX, int iBinY);
  int testImageCaptureContinous(int iNumFrame, int16 modeExposure, uns32 uExposureTime);
  int testImageCaptureFirstTime();
  int setupROI(rgn_type& region);
  int setupROI(rgn_type& region, int iRoiX, int iRoiY, int iRoiW, int iRoiH, int iBinX, int iBinY);
  // private data
  unsigned char* _pCurrentFrame;
  unsigned char* _pLockedFrame;
  bool _bLockFrame;

  // private static functions
  static int updateCameraSettings(int iReadoutPort, int iSpeedIndex, int iGainIndex, int iClearCycle, int iStrip, int iKinH, float fVsSpeed,
    int iCustW, int iCustH, int iTrgEdge);
  static int setupCooling(float fTemperature);
  static int printStatus();
  static int selectVsSpeed(float fRawVsSpeed);

public:
  static int16 getHcam() { return _hCam; }

private:
  static int16          _hCam;
};

int16 ImageCapture::_hCam = -1;

class TestDaqThread
{
public:
  int start();

private:
  static int createSendDataThread();
  static void *sendDataThread(void *);
};

static void showUsage()
{
    printf(
      "Usage:  princetonCameraTest  [-v|--version] [-h|--help] [-c|--camera <camera number>]\n"
      "    [-w|--write <filename prefix>] [-n|--number <number of images>] [-e|--exposure <exposure time>]\n"
      "    [-p|--port <Rport>] [-s|--speed <speed index>] [-g|--gain <gain index>]\n"
      "    [-t|--temp <temperature>] [-r|--roi <x,y,w,h>] [-b|--bin <xbin,ybin>]\n"
      "    [--kinh <height>] [--vshift <speed(us)>] [--clc <cycle>] [--strip <strip>]\n"
      "    [--custw <width>] [--custh <height>] [--trgmode <triggerMode>] [--trgedge <triggerEdge>]\n"
      "    [-m|--menu] \n"
      "  Options:\n"
      "    -v|--version                       Show file version\n"
      "    -h|--help                          Show usage\n"
      "    -c|--camera    <camera number>     Select camera\n"
      "    -w|--write     <filename prefix>   Output filename prefix\n"
      "    -n|--number    <number of images>  Number of images to be captured (Default: 1)\n"
      "    -e|--exposure  <exposure time>     Exposure time (in ms) (Default: 1 ms)\n"
      "    -p|--port      <readout port>      Readout port\n"
      "    -s|--speed     <speed inedx>       Speed table index\n"
      "    -g|--gain      <gain index>        Gain Index\n"
      "    -t|--temp      <temperature>       Temperature (in Celcius) (Default: 25 C)\n"
      "    -r|--roi       <x,y,w,h>           Region of Interest\n"
      "    -b|--bin       <xbin,ybin>         Binning of X/Y\n"
      "       --clc       <clear cycle>       Number of clear cycles\n"
      "       --strip     <strip per clear>   Number of rows per clear cycle\n"
      "       --kinh      <height>            Kinetics window height\n"
      "       --vss       <speed(us)>         Vertical shift speed\n"
      "       --custw     <width>             Custom detector width\n"
      "       --custh     <height>            Custom detector height\n"
      "       --trgmode   <triggerMode>       Trigger mode.\n"
      "                                         0: Timed 1: Strobed 2: Bulb 3: Trg_First\n"
      "       --trgedge   <triggerEdge>       Trigger edge: 0:+ 1:-\n"
      "    -m|--menu                          Show interactive menu\n"
    );
}

static void showVersion()
{
    printf( "Version:  princetonCameraTest  Ver %s\n", sPrincetonCameraTestVersion );
}

static int giExitAll = 0;
void signalIntHandler(int iSignalNo)
{
  printf( "\nsignalIntHandler(): signal %d received. Stopping all activities\n", iSignalNo );

  int16 hCam = ImageCapture::getHcam();
  if ( hCam != -1 )
  {
    rs_bool bStatus;
    int16   status;
    uns32   uNumBytesTransfered;
    bStatus = pl_exp_check_status(hCam, &status, &uNumBytesTransfered);
    printf( "exp status = %d\n", (int) status );

    if (!bStatus)
      printPvError("signalIntHandler():pl_exp_check_status() failed");
    else if ( status == EXPOSURE_IN_PROGRESS || status == READOUT_IN_PROGRESS || status == ACQUISITION_IN_PROGRESS )
    {
      bStatus = pl_exp_abort(hCam, CCS_HALT);

      if (!bStatus)
        printPvError("signalIntHandler():pl_exp_abort() failed");
    }
  }
  giExitAll = 1;
}

int main(int argc, char **argv)
{
  const char*         strOptions  = ":vhc:w:n:e:p:s:g:t:r:b:m";
  const struct option loOptions[] =
  {
     {"ver",      0, 0, 'v'},
     {"help",     0, 0, 'h'},
     {"camera",   1, 0, 'c'},
     {"write",    1, 0, 'w'},
     {"number",   1, 0, 'n'},
     {"exposure", 1, 0, 'e'},
     {"port",     1, 0, 'p'},
     {"speed",    1, 0, 's'},
     {"gain",     1, 0, 'g'},
     {"temp",     1, 0, 't'},
     {"roi",      1, 0, 'r'},
     {"bin",      1, 0, 'b'},
     {"menu",     0, 0, 'm'},
     {"clc",      1, 0,  1000},
     {"strip",    1, 0,  1001},
     {"kinh",     1, 0,  1002},
     {"vss",      1, 0,  1003},
     {"custw",    1, 0,  1004},
     {"custh",    1, 0,  1005},
     {"trgmode",  1, 0,  1006},
     {"trgedge",  1, 0,  1007},
     {0,          0, 0,  0 }
  };

  int   iCamera       = 0;
  char  sFnPrefix[32] = "";
  int   iNumImages    = 1;
  int   iExposureTime = 1;
  int   iReadoutPort  = -1;
  int   iSpeedIndex   = -1;
  int   iGainIndex    = -1;
  float fTemperature  = 25.0f;
  int   iRoiX         = 0;
  int   iRoiY         = 0;
  int   iRoiW         = -1;
  int   iRoiH         = -1;
  int   iBinX         = 1;
  int   iBinY         = 1;
  int   iClearCycle   = 0;
  int   iStrip        = 1;
  int   iKinH         = 0;
  int   iCustW        = 0;
  int   iCustH        = 0;
  int   iTrgMode      = 0;
  int   iTrgEdge      = 1;
  float fVsSpeed      = 0;
  int   iMenu         = 0;

  int iOptionIndex = 0;
  while ( int opt = getopt_long(argc, argv, strOptions, loOptions, &iOptionIndex ) )
  {
      if ( opt == -1 ) break;

      switch(opt)
      {
      case 'v':               /* Print usage */
          showVersion();
          return 0;
      case 'c':
          iCamera = strtoul(optarg, NULL, 0);
          break;
      case 'w':
          strcpy( sFnPrefix, optarg );
          break;
      case 'n':
          iNumImages    = strtoul(optarg, NULL, 0);
          break;
      case 'e':
          iExposureTime = strtoul(optarg, NULL, 0);
          break;
      case 'p':
          iReadoutPort  = strtoul(optarg, NULL, 0);
          break;
      case 's':
          iSpeedIndex   = strtoul(optarg, NULL, 0);
          break;
      case 'g':
          iGainIndex    = strtoul(optarg, NULL, 0);
          break;
      case 't':
          fTemperature  = strtof (optarg, NULL);
          break;
      case 'r':
          {
            char* pNextToken = optarg;
            iRoiX = strtoul(pNextToken, &pNextToken, 0); ++pNextToken;
            if ( *pNextToken == 0 ) break;
            iRoiY = strtoul(pNextToken, &pNextToken, 0); ++pNextToken;
            if ( *pNextToken == 0 ) break;
            iRoiW = strtoul(pNextToken, &pNextToken, 0); ++pNextToken;
            if ( *pNextToken == 0 ) break;
            iRoiH = strtoul(pNextToken, &pNextToken, 0); ++pNextToken;
            break;
          }
      case 'b':
          {
            char* pNextToken;
            iBinX = strtoul(optarg, &pNextToken, 0); ++pNextToken;
            if ( *pNextToken == 0 ) break;
            iBinY = strtoul(pNextToken, &pNextToken, 0); ++pNextToken;
            break;
          }
      case 'm':
          iMenu = 1;
          break;
      case 1000:
          iClearCycle   = strtoul(optarg, NULL, 0);
          break;
      case 1001:
          iStrip        = strtoul(optarg, NULL, 0);
          break;
      case 1002:
          iKinH         = strtoul(optarg, NULL, 0);
          break;
      case 1003:
          fVsSpeed     = strtof(optarg, NULL);
          break;
      case 1004:
          iCustW        = strtoul(optarg, NULL, 0);
          break;
      case 1005:
          iCustH        = strtoul(optarg, NULL, 0);
          break;
      case 1006:
          iTrgMode      = strtoul(optarg, NULL, 0);
          break;
      case 1007:
          iTrgEdge      = strtoul(optarg, NULL, 0);
          break;
      case '?':               /* Terse output mode */
          printf( "princetonCameraTest:main(): Unknown option: %c\n", optopt );
          break;
      case ':':               /* Terse output mode */
          printf( "princetonCameraTest:main(): Missing argument for %c\n", optopt );
          break;
      default:
      case 'h':               /* Print usage */
          showUsage();
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

  testPIDaq(iCamera, sFnPrefix, iNumImages, iExposureTime, iReadoutPort, iSpeedIndex, iGainIndex, fTemperature,
    iClearCycle, iStrip, iKinH, fVsSpeed, iCustW, iCustH, iTrgEdge,
    iRoiX, iRoiY, iRoiW, iRoiH, iBinX, iBinY, iTrgMode, iMenu);
}

int testPIDaq(int iCamera, char* sFnPrefix, int iNumImages, int iExposureTime, int iReadoutPort, int iSpeedIndex,
  int iGainIndex, float fTemperature, int iClearCycle, int iStrip, int iKinH, float fVsSpeed, int iCustW, int iCustH, int iTrgEdge,
  int iRoiX, int iRoiY, int iRoiW, int iRoiH, int iBinX, int iBinY, int iTrgMode,
  int iMenu)
{
  ImageCapture  imageCapure;
  TestDaqThread testDaqThread;

  int iFail = testDaqThread.start();
  if (0 != iFail)
  {
    printf("testPIDaq(): testDaqThread.start() failed, error code = %d\n", iFail);
    return 1;
  }

  iFail = imageCapure.start(iCamera, sFnPrefix, iNumImages, iExposureTime, iReadoutPort, iSpeedIndex,
    iGainIndex, fTemperature, iClearCycle, iStrip, iKinH, fVsSpeed, iCustW, iCustH, iTrgEdge,
    iRoiX, iRoiY, iRoiW, iRoiH, iBinX, iBinY, iTrgMode, iMenu);
  if (0 != iFail)
  {
    printf("testPIDaq(): imageCapure.start() failed, error code = %d\n", iFail);
    return 1;
  }

  return 0;
}

int TestDaqThread::start()
{
  int iFail = createSendDataThread();
  if (0 != iFail)
  {
    printf("TestDaqThread::start(): createSendDataThread() failed, error code = %d\n", iFail);
    return 1;
  }

  return 0;
}

int TestDaqThread::createSendDataThread()
{
  pthread_attr_t threadAttr;
  pthread_attr_init(&threadAttr);
  pthread_attr_setdetachstate(&threadAttr, PTHREAD_CREATE_DETACHED);

  pthread_t threadSendData;
  int iFail =
    pthread_create(&threadSendData, &threadAttr, &sendDataThread, NULL);
  if (iFail != 0)
  {
    printf("TestDaqThread::createSendDataThread(): pthread_create() failed, error code %d - %s\n",
       errno, strerror(errno));
    return 1;
  }

  return 0;
}

void* TestDaqThread::sendDataThread(void *)
{
   while (1)
   {
     sleep(1);
   }

   return NULL;
}

int ImageCapture::updateCameraSettings(int iReadoutPort, int iSpeedIndex, int iGainIndex, int iClearCycle, int iStrip, int iKinH, float fVsSpeed,
  int iCustW, int iCustH, int iTrgEdge)
{
  using PICAM::displayParamIdInfo;
  displayParamIdInfo(_hCam, PARAM_EXPOSURE_MODE, "Exposure Mode");
  displayParamIdInfo(_hCam, PARAM_SHTR_OPEN_MODE, "Shutter Open Mode");
  displayParamIdInfo(_hCam, PARAM_SHTR_OPEN_DELAY, "Shutter Open Delay");
  displayParamIdInfo(_hCam, PARAM_SHTR_CLOSE_DELAY, "Shutter Close Delay");
  displayParamIdInfo(_hCam, PARAM_EDGE_TRIGGER, "Edge Trigger" );

  displayParamIdInfo(_hCam, PARAM_EXP_RES, "Exposure Resolution");
  displayParamIdInfo(_hCam, PARAM_EXP_RES_INDEX, "Exposure Resolution Index");

  displayParamIdInfo(_hCam, PARAM_CLEAR_MODE  , "Clear Mode");

  if ( iClearCycle >= 0 )
  {
    displayParamIdInfo(_hCam, PARAM_CLEAR_CYCLES, "Clear Cycle *org*");
    int16 i16ClearCycle = (int16) iClearCycle;
    PICAM::setAnyParam(_hCam, PARAM_CLEAR_CYCLES, &i16ClearCycle );
  }
  displayParamIdInfo(_hCam, PARAM_CLEAR_CYCLES, "Clear Cycles");

  if ( iStrip >= 0 )
  {
    displayParamIdInfo(_hCam, PARAM_NUM_OF_STRIPS_PER_CLR, "Strips Per Clear *org*");
    int16 i16Strip = (int16) iStrip;
    PICAM::setAnyParam(_hCam, PARAM_NUM_OF_STRIPS_PER_CLR, &i16Strip );
  }
  displayParamIdInfo(_hCam, PARAM_NUM_OF_STRIPS_PER_CLR, "Strips Per Clear");


  displayParamIdInfo(_hCam, PARAM_MIN_BLOCK    , "Min Block Size");
  displayParamIdInfo(_hCam, PARAM_NUM_MIN_BLOCK, "Num of Min Block");

  if ( iReadoutPort >= 0 )
  {
    displayParamIdInfo(_hCam, PARAM_READOUT_PORT, "Readout port *org*");
    uns32 u32ReadoutPort = (uns32) iReadoutPort;
    PICAM::setAnyParam(_hCam, PARAM_READOUT_PORT, &u32ReadoutPort );
  }
  displayParamIdInfo(_hCam, PARAM_READOUT_PORT, "Readout port");

  if ( iSpeedIndex >= 0 )
  {
    displayParamIdInfo(_hCam, PARAM_SPDTAB_INDEX, "Speed Table Index *org*");
    int16 i16SpeedTableIndex = iSpeedIndex;
    PICAM::setAnyParam(_hCam, PARAM_SPDTAB_INDEX, &i16SpeedTableIndex );
  }
  displayParamIdInfo(_hCam, PARAM_SPDTAB_INDEX, "Speed Table Index");

  if ( iGainIndex >= 0 )
  {
    displayParamIdInfo(_hCam, PARAM_GAIN_INDEX, "Gain Index *org*");
    int16 i16GainIndex = iGainIndex;
    PICAM::setAnyParam(_hCam, PARAM_GAIN_INDEX, &i16GainIndex );
  }
  displayParamIdInfo(_hCam, PARAM_GAIN_INDEX  , "Gain Index");

  displayParamIdInfo(_hCam, PARAM_PIX_TIME    , "Pixel Transfer Time");
  displayParamIdInfo(_hCam, PARAM_BIT_DEPTH   , "Bit Depth");

  displayParamIdInfo(_hCam, PARAM_LOGIC_OUTPUT, "Logic Output *org*");
  uns32 u32LogicOutput = (uns32) OUTPUT_NOT_SCAN;
  PICAM::setAnyParam(_hCam, PARAM_LOGIC_OUTPUT, &u32LogicOutput );
  displayParamIdInfo(_hCam, PARAM_LOGIC_OUTPUT, "Logic Output *new*");

  displayParamIdInfo(_hCam, PARAM_PMODE, "P Mode *org*");
  uns32 u32pmode = ((iKinH != 0)? (uns32) PMODE_KINETICS : (uns32) PMODE_NORMAL);
  PICAM::setAnyParam(_hCam, PARAM_PMODE, &u32pmode );
  displayParamIdInfo(_hCam, PARAM_PMODE, "P Mode");

  if (iKinH != 0)
  {
    displayParamIdInfo(_hCam, PARAM_KIN_WIN_SIZE, "Kinetics Win Height *org*");
    uns16 u16kinSize = (uns16) iKinH;
    PICAM::setAnyParam(_hCam, PARAM_KIN_WIN_SIZE, &u16kinSize );
  }
  displayParamIdInfo(_hCam, PARAM_KIN_WIN_SIZE, "Kinetics Win Height");

  displayParamIdInfo(_hCam, PARAM_CUSTOM_TIMING, "Custom Timing *org*");
  rs_bool iCustomTiming = ( fVsSpeed != 0 ? 1 : 0);
  PICAM::setAnyParam(_hCam, PARAM_CUSTOM_TIMING, &iCustomTiming );
  displayParamIdInfo(_hCam, PARAM_CUSTOM_TIMING, "Custom Timing");

  if (fVsSpeed != 0)
  {
    displayParamIdInfo(_hCam, PARAM_PAR_SHIFT_TIME, "Vertical Shift Speed *org*");
    int32 i32parShiftTime = selectVsSpeed(fVsSpeed * 1000);
    PICAM::setAnyParam(_hCam, PARAM_PAR_SHIFT_TIME, &i32parShiftTime );
  }
  displayParamIdInfo(_hCam, PARAM_PAR_SHIFT_TIME, "Vertical Shift Speed");

  displayParamIdInfo(_hCam, PARAM_SER_SHIFT_TIME, "Serial Shift Speed");

  if (iCustW != 0 || iCustH != 0 )
  {
    rs_bool iCustomChip = 1;
    PICAM::setAnyParam(_hCam, PARAM_CUSTOM_CHIP, &iCustomChip );

    if (iCustW != 0)
    {
      displayParamIdInfo(_hCam, PARAM_SER_SIZE, "Detecotr Width *org*");
      uns16 u16CustomW = (uns16) iCustW;
      PICAM::setAnyParam(_hCam, PARAM_SER_SIZE, &u16CustomW );
    }
    else
    {
      uns16 u16CustomH = 0;
      PICAM::getAnyParam(_hCam, PARAM_SER_SIZE, &u16CustomH, ATTR_DEFAULT );
      PICAM::setAnyParam(_hCam, PARAM_SER_SIZE, &u16CustomH );
    }

    if (iCustH != 0)
    {
      displayParamIdInfo(_hCam, PARAM_PAR_SIZE, "Detecotr Height *org*");
      uns16 u16CustomH = (uns16) iCustH;
      PICAM::setAnyParam(_hCam, PARAM_PAR_SIZE, &u16CustomH );
    }
    else
    {
      uns16 u16CustomH = 0;
      PICAM::getAnyParam(_hCam, PARAM_PAR_SIZE, &u16CustomH, ATTR_DEFAULT );
      PICAM::setAnyParam(_hCam, PARAM_PAR_SIZE, &u16CustomH );
    }

    rs_bool iSkipSRegClean = 1;
    PICAM::setAnyParam(_hCam, PARAM_SKIP_SREG_CLEAN, &iSkipSRegClean );
  }
  else
  {
    rs_bool iCustomChip = 1;
    PICAM::setAnyParam(_hCam, PARAM_CUSTOM_CHIP, &iCustomChip );

    uns16 u16CustomW = 0;
    PICAM::getAnyParam(_hCam, PARAM_SER_SIZE, &u16CustomW, ATTR_DEFAULT );
    PICAM::setAnyParam(_hCam, PARAM_SER_SIZE, &u16CustomW );

    uns16 u16CustomH = 0;
    PICAM::getAnyParam(_hCam, PARAM_PAR_SIZE, &u16CustomH, ATTR_DEFAULT );
    PICAM::setAnyParam(_hCam, PARAM_PAR_SIZE, &u16CustomH );

    rs_bool iSkipSRegClean = 0;
    PICAM::setAnyParam(_hCam, PARAM_SKIP_SREG_CLEAN, &iSkipSRegClean );

    iCustomChip = 0;
    PICAM::setAnyParam(_hCam, PARAM_CUSTOM_CHIP, &iCustomChip );
  }
  displayParamIdInfo(_hCam, PARAM_CUSTOM_CHIP, "Custom Chip");
  displayParamIdInfo(_hCam, PARAM_SER_SIZE, "Detecotr Width");
  displayParamIdInfo(_hCam, PARAM_PAR_SIZE, "Detecotr Height");
  displayParamIdInfo(_hCam, PARAM_SKIP_SREG_CLEAN, "Skip Serial Reg Clean");

  if (iTrgEdge >= 0)
  {
    displayParamIdInfo(_hCam, PARAM_EDGE_TRIGGER, "Trigger Edge *org*");
    uns32 u32TriggerEdge = (uns32) (iTrgEdge == 0 ? EDGE_TRIG_POS : EDGE_TRIG_NEG);
    PICAM::setAnyParam(_hCam, PARAM_EDGE_TRIGGER, &u32TriggerEdge );
  }
  displayParamIdInfo(_hCam, PARAM_EDGE_TRIGGER, "Trigger Edge");

  return 0;
}

int ImageCapture::setupCooling(float fTemperature)
{
  using namespace PICAM;
  // Cooling settings

  displayParamIdInfo(_hCam, PARAM_COOLING_MODE, "Cooling Mode");

  int16 iCoolingTemp = (int) (fTemperature * 100);

  if ( iCoolingTemp > 2500 )
  {
    printf( "Skip cooling, since the cooling temperature (%.1f C) is larger than max value (25 C)\n",
      iCoolingTemp/100.f );
    return 0;
  }

  int16 iTemperatureCurrent = -1;
  getAnyParam( _hCam, PARAM_TEMP, &iTemperatureCurrent );

  displayParamIdInfo( _hCam, PARAM_TEMP, "Temperature Before Cooling" );

  setAnyParam( _hCam, PARAM_TEMP_SETPOINT, &iCoolingTemp );
  displayParamIdInfo( _hCam, PARAM_TEMP_SETPOINT, "Set Cooling Temperature" );


  const int iMaxCoolingTime = 60000; // in miliseconds

  timeval timeSleepMicroOrg = {0, 5000 }; // 5 milliseconds
  timespec timeVal1;
  clock_gettime( CLOCK_REALTIME, &timeVal1 );

  int iNumLoop       = 0;
  int iNumRepateRead = 3;
  int iRead          = 0;

  /* wait for data or error */
  while (1)
  {
    getAnyParam( _hCam, PARAM_TEMP,          &iTemperatureCurrent );

    if ( iTemperatureCurrent <= iCoolingTemp )
    {
      if ( ++iRead >= iNumRepateRead )
        break;
    }
    else
      iRead = 0;

    if ( (iNumLoop+1) % 200 == 0 )
      displayParamIdInfo(_hCam, PARAM_TEMP, "Temperature *Updating*" );

    timespec timeValCur;
    clock_gettime( CLOCK_REALTIME, &timeValCur );
    int iWaitTime = (timeValCur.tv_nsec - timeVal1.tv_nsec) / 1000000 +
     ( timeValCur.tv_sec - timeVal1.tv_sec ) * 1000; // in milliseconds
    if ( iWaitTime > iMaxCoolingTime ) break;

    // This data will be modified by select(), so need to be reset
    timeval timeSleepMicro = timeSleepMicroOrg;
    // use select() to simulate nanosleep(), because experimentally select() controls the sleeping time more precisely
    select( 0, NULL, NULL, NULL, &timeSleepMicro);
    iNumLoop++;
  }

  timespec timeVal2;
  clock_gettime( CLOCK_REALTIME, &timeVal2 );
  double fCoolingTime = (timeVal2.tv_nsec - timeVal1.tv_nsec) * 1.e-6 + ( timeVal2.tv_sec - timeVal1.tv_sec ) * 1.e3;
  printf("Cooling Time = %6.1lf ms\n", fCoolingTime);

  displayParamIdInfo( _hCam, PARAM_TEMP, "Final Temperature" );

  return 0;
}

int ImageCapture::selectVsSpeed(float fRawVsSpeed)
{
  int32 i32parShiftTimeMin, i32parShiftTimeMax, i32parShiftTimeInc;
  PICAM::getAnyParam(_hCam, PARAM_PAR_SHIFT_TIME, &i32parShiftTimeMin, ATTR_MIN );
  PICAM::getAnyParam(_hCam, PARAM_PAR_SHIFT_TIME, &i32parShiftTimeMax, ATTR_MAX );
  PICAM::getAnyParam(_hCam, PARAM_PAR_SHIFT_TIME, &i32parShiftTimeInc, ATTR_INCREMENT );

  float fRatio = (fRawVsSpeed - i32parShiftTimeMin) / i32parShiftTimeInc;
  int i32parShiftTime = i32parShiftTimeMin + i32parShiftTimeInc * (int)(fRatio +  0.5);

  if (i32parShiftTime <= i32parShiftTimeMin)
    return i32parShiftTimeMin;
  if (i32parShiftTime >= i32parShiftTimeMax)
    return i32parShiftTimeMax;

  return (int) i32parShiftTime;
}

using namespace std;

int ImageCapture::start(int iCamera, char* sFnPrefix, int iNumImages, int iExposureTime, int iReadoutPort, int iSpeedIndex,
  int iGainIndex, float fTemperature, int iClearCycle, int iStrip, int iKinH, float fVsSpeed, int iCustW, int iCustH, int iTrgEdge,
  int iRoiX, int iRoiY, int iRoiW, int iRoiH, int iBinX, int iBinY, int iTrgMode, int iMenu)
{
  char cam_name[CAM_NAME_LEN];  /* camera name                    */

  rs_bool bStatus;

  timespec timeVal0;
  clock_gettime( CLOCK_REALTIME, &timeVal0 );

  /* Initialize the PVCam Library and Open the First Camera */
  bStatus = pl_pvcam_init();
  if (!bStatus)
  {
    printPvError("ImageCapture::start(): pl_pvcam_init() failed");
    return 1;
  }

  int16 iNumCamera = 0;
  bStatus = pl_cam_get_total(&iNumCamera);
  if (!bStatus)
  {
    printPvError("ImageCapture::start(): pl_cam_get_total() failed");
    return 3;
  }

  bStatus = pl_cam_get_name(iCamera, cam_name);
  if (!bStatus)
  {
    printPvError("ImageCapture::start(): pl_cam_get_name() failed");
    return 5;
  }
  printf( "Using Camera Serial %d (Total %d)  Name %s\n", iCamera, iNumCamera, cam_name );

  bStatus = pl_cam_open(cam_name, &_hCam, OPEN_EXCLUSIVE);
  if (!bStatus)
  {
    printPvError("ImageCapture::start(): pl_cam_open() failed");
    return 6;
  }

  timespec timeVal1;
  clock_gettime( CLOCK_REALTIME, &timeVal1 );
  double fOpenTime = (timeVal1.tv_nsec - timeVal0.tv_nsec) * 1.e-6 + ( timeVal1.tv_sec - timeVal0.tv_sec ) * 1.e3;
  printf("Camera Open Time = %6.1lf ms\n", fOpenTime);

  int16 i16MaxSpeedTableIndex = -1;
  int16 i16MaxGainIndex       = -1;
  uns32 u32MaxReadoutPort     = 0;
  PICAM::getAnyParam(_hCam, PARAM_SPDTAB_INDEX, &i16MaxSpeedTableIndex, ATTR_MAX);
  PICAM::getAnyParam(_hCam, PARAM_GAIN_INDEX, &i16MaxGainIndex, ATTR_MAX, 0);
  PICAM::getAnyParam(_hCam, PARAM_READOUT_PORT, &u32MaxReadoutPort, ATTR_MAX);
  if (iSpeedIndex < 0)
    iSpeedIndex = i16MaxSpeedTableIndex;
  if (iGainIndex < 0)
    iGainIndex  = i16MaxGainIndex;
  if (iReadoutPort < 0)
    iReadoutPort = u32MaxReadoutPort;

  uns16 u16Width, u16Height;
  PICAM::getAnyParam( _hCam, PARAM_SER_SIZE, &u16Width );
  PICAM::getAnyParam( _hCam, PARAM_PAR_SIZE, &u16Height );
  if (iRoiW < 0 )
    iRoiW = u16Width;
  if (iRoiH < 0 )
    iRoiH = u16Height;

  printStatus();

  setupCooling(fTemperature);

  updateCameraSettings(iReadoutPort, iSpeedIndex, iGainIndex, iClearCycle, iStrip, iKinH, fVsSpeed, iCustW, iCustH, iTrgEdge);

  printStatus();

  testImageCaptureFirstTime(); // dummy init capture

  string  strFnPrefix(sFnPrefix);
  float   fExposureTime = iExposureTime * 0.001;
  bool bQuitCapture = false;
  while (true)
  {
    if (iMenu != 0)
    {
      bool bQuitMenu = false;
      do
      {
        //Show menu options
        cout << "======= Current Configuration ======="  << endl;
        printStatus();
        cout << "Output Filename Prefix   : " << strFnPrefix   << endl;
        cout << "Number Images per Acq    : " << iNumImages    << endl;
        cout << "Exposure Time (sec)      : " << fExposureTime << endl;
        cout << "Readout Port             : " << iReadoutPort  << endl;
        cout << "Speed Index              : " << iSpeedIndex   <<  endl;
        cout << "Gain Index               : " << iGainIndex    << endl;
        cout << "Cooling Temperature (C)  : " << fTemperature  << endl;
        cout << "Clear Cycle              : " << iClearCycle   << endl;
        cout << "Strip per Clear          : " << iStrip        << endl;
        cout << "Kinetcis Win Height      : " << iKinH         << endl;
        cout << "Vertical Shift Speed     : " << fVsSpeed      << endl;
        cout << "Trigger Mode             : " << iTrgMode      << endl;
        cout << "Trigger Edge             : " << iTrgEdge      << endl;
        cout << "Custom Width             : " << iCustW        << endl;
        cout << "Custom Height            : " << iCustH        << endl;
        cout << "ROI : x " << iRoiX << " y " <<  iRoiY <<
          " W " << iRoiW << " H " << iRoiH <<
          " binX " << iBinX << " binY " << iBinY << endl;
        cout << "=============== Menu ================"  << endl;
        cout << "a. Start Acquisition" << endl;
        cout << "w. Set Output Filename Prefix" << endl;
        cout << "n. Set Number of Images per Acquisition" << endl;
        cout << "e. Set Exposure Time" << endl;
        cout << "p. Set Readout Port" << endl;
        cout << "s. Set Speed Index" << endl;
        cout << "g. Set Gain Index" << endl;
        cout << "t. Set Cooling Temperature (C)" << endl;
        cout << "k. Set Kinetics Win Height" << endl;
        cout << "h. Set Vertical Shift Speed (us)" << endl;
        cout << "1. Set Trigger Mode" << endl;
        cout << "2. Set Trigger Edge" << endl;
        cout << "3. Set Clear Cycle" << endl;
        cout << "4. Set Strip per Clear" << endl;
        cout << "5. Set Custom Width" << endl;
        cout << "6. Set Custom Height" << endl;
        cout << "r. Set ROI" << endl;
        cout << "b. Set Binning" << endl;
        cout << "q. Quit Program" << endl;
        cout << "====================================="  << endl;
        cout << "> ";
        //Get menu choice
        int choice = getchar();

        bool bReConfig = false;
        switch(choice)
        {
        case 'a': //Acquire
          bQuitMenu = true;
          break;
        case 'w':
        {
          cout << endl << "Enter new Output Filename Prefix ('x': disable output) > ";
          cin >> strFnPrefix;
          if (strFnPrefix == "x")
            strFnPrefix = "";
          printf("New Output Filename Prefix: \'%s\'\n", strFnPrefix.c_str());
          break;
        }
        case 'n':
        {
          cout << endl << "Enter new Number of Images per Acquisition > ";
          cin >> iNumImages;
          printf("New Number of Images per Acquisition: %d\n", iNumImages);
          break;
        }
        case 'e':
        {
          cout << endl << "Enter new Exposure Time (sec) > ";
          cin >> fExposureTime;
          printf("New Exposure Time: %f\n", fExposureTime);
          iExposureTime = (int)(fExposureTime * 1000);
          break;
        }
        case 'p':
        {
          cout << endl << "Enter new Readout Port > ";
          cin >> iReadoutPort;
          printf("New Readout Port: %d\n", iReadoutPort);
          bReConfig = true;
          break;
        }
        case 's':
        {
          cout << endl << "Enter new Speed Index > ";
          cin >> iSpeedIndex;
          printf("New Speed Index: %d\n", iSpeedIndex);
          bReConfig = true;
          break;
        }
        case 'g':
        {
          cout << endl << "Enter new Gain Index > ";
          cin >> iGainIndex;
          printf("New Gain Index: %d\n", iGainIndex);
          bReConfig = true;
          break;
        }
        case 't':
        {
          cout << endl << "Enter new Cooling Temperature (C) > ";
          cin >> fTemperature;
          printf("New Cooling Temperature: %f\n", fTemperature);
          setupCooling(fTemperature);
          break;
        }
        case 'k':
        {
          cout << endl << "Enter new Kinetics Win Height > ";
          cin >> iKinH;
          printf("New Kinetics Win Height: %d\n", iKinH);
          bReConfig = true;
          break;
        }
        case 'h':
        {
          cout << endl << "Enter new Veritical Shift Speed (us) > ";
          cin >> fVsSpeed;
          printf("New Veritical Shift Speed (us) : %f\n", fVsSpeed);
          bReConfig = true;
          break;
        }
        case '1':
        {
          cout << endl << "Enter new Trigger Mode > ";
          cin >> iTrgMode;
          printf("New Trigger Mode: %d\n", iTrgMode);
          break;
        }
        case '2':
        {
          cout << endl << "Enter new Trigger Edge > ";
          cin >> iTrgEdge;
          printf("New Trigger Edge: %d\n", iTrgEdge);
          bReConfig = true;
          break;
        }
        case '3':
        {
          cout << endl << "Enter new Clear Cycle > ";
          cin >> iClearCycle;
          printf("New Clear Cycle: %d\n", iClearCycle);
          bReConfig = true;
          break;
        }
        case '4':
        {
          cout << endl << "Enter new Strips per Clear > ";
          cin >> iStrip;
          printf("New Strips per Clear: %d\n", iStrip);
          bReConfig = true;
          break;
        }
        case '5':
        {
          cout << endl << "Enter new Custom Width > ";
          cin >> iCustW;
          printf("New Custom Width Width: %d\n", iCustW);
          bReConfig = true;
          break;
        }
        case '6':
        {
          cout << endl << "Enter new Custom Height > ";
          cin >> iCustH;
          printf("New Custom Height: %d\n", iCustH);
          bReConfig = true;
          break;
        }
        case 'r':
        {
          cout << endl << "Enter new ROI [x,y,W,H] > ";
          string strROI;
          cin >> strROI;
          char sROI[64];
          strcpy(sROI, strROI.c_str());
          char* pNextToken = sROI;
          iRoiX = strtoul(pNextToken, &pNextToken, 0); ++pNextToken;
          if ( *pNextToken == 0 ) break;
          iRoiY = strtoul(pNextToken, &pNextToken, 0); ++pNextToken;
          if ( *pNextToken == 0 ) break;
          iRoiW = strtoul(pNextToken, &pNextToken, 0); ++pNextToken;
          if ( *pNextToken == 0 ) break;
          iRoiH = strtoul(pNextToken, &pNextToken, 0); ++pNextToken;
          printf("New ROI: x %d y %d W %d H %d\n", iRoiX, iRoiY, iRoiW, iRoiH);
          break;
        }
        case 'b':
        {
          cout << endl << "Enter new Binning [binX,binY] > ";
          string strBin;
          cin >> strBin;
          char sBin[64];
          strcpy(sBin, strBin.c_str());
          char* pNextToken = sBin;
          iBinX = strtoul(pNextToken, &pNextToken, 0); ++pNextToken;
          if ( *pNextToken == 0 ) break;
          iBinY = strtoul(pNextToken, &pNextToken, 0); ++pNextToken;
          printf("New binX %d binY %d \n", iBinX, iBinY);
          break;
        }
        case 'q': //Exit
          bQuitMenu     = true;
          bQuitCapture  = true;
          break;

        default:
          cout << "!Invalid Option! Key = " << choice << endl;
        }
        getchar();

        if (bReConfig)
          updateCameraSettings(iReadoutPort, iSpeedIndex, iGainIndex, iClearCycle, iStrip, iKinH, fVsSpeed, iCustW, iCustH, iTrgEdge);
      }
      while (!bQuitMenu);
    } // if (iMenu != 0)

    if (bQuitCapture)
      break;

    const int iNumFrame         = iNumImages;
    const int16 modeExposure    = iTrgMode;
    //const int16 modeExposure = STROBED_MODE;
    const uns32 u32ExposureTime = iExposureTime;

    testImageCaptureStandard(strFnPrefix.c_str(), iNumFrame, modeExposure, u32ExposureTime, iRoiX, iRoiY, iRoiW, iRoiH, iBinX, iBinY);

    if (iMenu == 0)
      break;
  }

  printStatus();

  //testImageCaptureContinous(hCam, iNumFrame, modeExposure, u32ExposureTime);

  bStatus = pl_cam_close(_hCam);
  if (!bStatus)
  {
    printPvError("ImageCapture::start(): pl_cam_close() failed");
    return 7;
  }

  bStatus = pl_pvcam_uninit();
  if (!bStatus)
  {
    printPvError("ImageCapture::start(): pl_pvcam_uninit() failed");
    return 8;
  }

  return 0;
}

int ImageCapture::setupROI(rgn_type& region)
{
  using PICAM::getAnyParam;
  uns16 u16Width, u16Height;
  PICAM::getAnyParam( _hCam, PARAM_SER_SIZE, &u16Width );
  PICAM::getAnyParam( _hCam, PARAM_PAR_SIZE, &u16Height );

  // set the region to use full frame and 1x1 binning
  region.s1 = 0;
  region.s2 = u16Width - 1;
  region.sbin = 1;
  region.p1 = 0;
  region.p2 = u16Height - 1;
  region.pbin = 1;

  return 0;
}

int ImageCapture::setupROI(rgn_type& region, int iRoiX, int iRoiY, int iRoiW, int iRoiH, int iBinX, int iBinY)
{
  using PICAM::getAnyParam;
  uns16 u16Width, u16Height;
  PICAM::getAnyParam( _hCam, PARAM_SER_SIZE, &u16Width );
  PICAM::getAnyParam( _hCam, PARAM_PAR_SIZE, &u16Height );

  if (iRoiX < 0)
    iRoiX = 0;
  if (iRoiY < 0)
    iRoiY = 0;

  // set the region to use full frame and 1x1 binning
  region.s1 = iRoiX;
  region.s2 = iRoiX + iRoiW - 1;
  region.sbin = iBinX;
  region.p1 = iRoiY;
  region.p2 = iRoiY + iRoiH - 1;
  region.pbin = iBinY;

  if (region.s2 >= u16Width)
    region.s2 = u16Width-1;
  if (region.p2 >= u16Height)
    region.p2 = u16Height-1;

  return 0;
}


int ImageCapture::testImageCaptureFirstTime()
{
  rgn_type region;
  setupROI(region);
  region.s1   = 0;
  region.s2   = 127;
  region.sbin = 1;
  region.p1   = 0;
  region.p2   = 127;
  region.pbin = 1;

  PICAM::printROI(1, &region);

  timespec timeVal0;
  clock_gettime( CLOCK_REALTIME, &timeVal0 );

  /* Init a sequence set the region, exposure mode and exposure time */
  pl_exp_init_seq();

  uns32 uFrameSize = 0;
  pl_exp_setup_seq(_hCam, 1, 1, &region, TIMED_MODE, 1, &uFrameSize);
  uns16* pFrameBuffer = (uns16 *) malloc(uFrameSize);
  printf( "frame size for first capture = %lu\n", uFrameSize );

  double fReadoutTime = -1;
  PICAM::getAnyParam(_hCam, PARAM_READOUT_TIME, &fReadoutTime);
  printf("Estimated Readout Time = %.1lf ms\n", fReadoutTime);

  timeval timeSleepMicroOrg = {0, 1000 }; // 1 milliseconds

  /* ACQUISITION LOOP */
  if (giExitAll != 0)
    return 0;

  /* frame now contains valid data */
  printf( "Taking test frame... " );
  fflush(NULL);

  timespec timeVal1;
  clock_gettime( CLOCK_REALTIME, &timeVal1 );

  pl_exp_start_seq(_hCam, pFrameBuffer);

  timespec timeVal2;
  clock_gettime( CLOCK_REALTIME, &timeVal2 );

  uns32 uNumBytesTransfered;
  int iNumLoop = 0;
  int16 status = 0;

  /* wait for data or error */
  while (pl_exp_check_status(_hCam, &status, &uNumBytesTransfered) &&
         (status != READOUT_COMPLETE && status != READOUT_FAILED))
  {
    // This data will be modified by select(), so need to be reset
    timeval timeSleepMicro = timeSleepMicroOrg;
    // use select() to simulate nanosleep(), because experimentally select() controls the sleeping time more precisely
    select( 0, NULL, NULL, NULL, &timeSleepMicro);
    iNumLoop++;
  }

  /* Check Error Codes */
  if (status == READOUT_FAILED)
    printPvError("ImageCapture::testImageCaptureFirstTime():pl_exp_check_status() failed");

  timespec timeVal3;
  clock_gettime( CLOCK_REALTIME, &timeVal3 );
  double fInitTime    = (timeVal1.tv_nsec - timeVal0.tv_nsec) * 1.e-6 + ( timeVal1.tv_sec - timeVal0.tv_sec ) * 1.e3;
  double fStartupTime = (timeVal2.tv_nsec - timeVal1.tv_nsec) * 1.e-6 + ( timeVal2.tv_sec - timeVal1.tv_sec ) * 1.e3;
  double fPollingTime = (timeVal3.tv_nsec - timeVal2.tv_nsec) * 1.e-6 + ( timeVal3.tv_sec - timeVal2.tv_sec ) * 1.e3;
  double fSingleFrameTime = fStartupTime + fPollingTime;
  printf("Catpure Init Time = %6.1lf Startup Time = %6.1lf Polling Time = %7.1lf Frame Time = %.1lf ms\n",
    fInitTime, fStartupTime, fPollingTime, fSingleFrameTime );


  // pl_exp_finish_seq(hCam, pFrameBuffer, 0); // No need to call this function, unless we have multiple ROIs

  /*Uninit the sequence */
  pl_exp_uninit_seq();

  free(pFrameBuffer);

  return 0;
}

int ImageCapture::testImageCaptureStandard(const char* sFnPrefix, int iNumFrame, int16 modeExposure, uns32 uExposureTime,
  int iRoiX, int iRoiY, int iRoiW, int iRoiH, int iBinX, int iBinY)
{
  printf( "Starting standard image capture for %d frames, exposure mode = %d, exposure time = %d ms\n",
    iNumFrame, (int) modeExposure, (int) uExposureTime);

  rgn_type region;
  if (iRoiW < 0 || iRoiH < 0 )
    setupROI(region);
  else
    setupROI(region, iRoiX, iRoiY, iRoiW, iRoiH, iBinX, iBinY);

  PICAM::printROI(1, &region);

  timespec timeVal0;
  clock_gettime( CLOCK_REALTIME, &timeVal0 );

  /* Init a sequence set the region, exposure mode and exposure time */
  pl_exp_init_seq();

  uns32 uFrameSize = 0;
  pl_exp_setup_seq(_hCam, 1, 1, &region, modeExposure, uExposureTime, &uFrameSize);
  uns16* pFrameBuffer = (uns16 *) malloc(uFrameSize);
  printf( "frame size for standard capture = %lu\n", uFrameSize );

  timeval timeSleepMicroOrg = {0, 1000 }; // 1 milliseconds

  if (modeExposure == STROBED_MODE)
    PICAM::displayParamIdInfo(_hCam, PARAM_CONT_CLEARS , "Continuous Clearing");

  double fReadoutTime = -1;
  PICAM::getAnyParam(_hCam, PARAM_READOUT_TIME, &fReadoutTime);
  printf("Estimated Readout Time = %.1lf ms\n", fReadoutTime);

  /* Start the acquisition */
  printf("Collecting %i Frames\n", iNumFrame);

  timespec timeVal0a;
  clock_gettime( CLOCK_REALTIME, &timeVal0a );
  double  fInitTime = (timeVal0a.tv_nsec - timeVal0.tv_nsec) * 1.e-6 + ( timeVal0a.tv_sec - timeVal0.tv_sec ) * 1.e3;

  double fAvgStartupTime = 0, fAvgPollingTime = 0, fAvgSingleFrameTime = 0, fAvgAvgVal = 0, fAvgStdVal = 0;
  double fSumSingleFrameTimeSq = 0;
  double fMaxSingleFrameTime   = 0;
  /* ACQUISITION LOOP */
  for (int iFrame = 0; iFrame < iNumFrame; iFrame++)
  {
    if (giExitAll != 0)
    {
      iNumFrame = iFrame;
      break;
    }

    /* frame now contains valid data */
    printf( "Taking frame %d... ", iFrame );
    fflush(NULL);

    timespec timeVal1;
    clock_gettime( CLOCK_REALTIME, &timeVal1 );
    if (fInitTime == -1)
      fInitTime = (timeVal1.tv_nsec - timeVal0.tv_nsec) * 1.e-6 + ( timeVal1.tv_sec - timeVal0.tv_sec ) * 1.e3;

    pl_exp_start_seq(_hCam, pFrameBuffer);

    timespec timeVal2;
    clock_gettime( CLOCK_REALTIME, &timeVal2 );

    uns32 uNumBytesTransfered;
    int iNumLoop = 0;
    int16 status = 0;

    /* wait for data or error */
    while (pl_exp_check_status(_hCam, &status, &uNumBytesTransfered) &&
           (status != READOUT_COMPLETE && status != READOUT_FAILED))
    {
      // This data will be modified by select(), so need to be reset
      timeval timeSleepMicro = timeSleepMicroOrg;
      // use select() to simulate nanosleep(), because experimentally select() controls the sleeping time more precisely
      select( 0, NULL, NULL, NULL, &timeSleepMicro);
      iNumLoop++;
    }

    /* Check Error Codes */
    if (status == READOUT_FAILED)
    {
      printPvError("ImageCapture::testImageCaptureStandard():pl_exp_check_status() failed");
      iNumFrame = iFrame;
      break;
    }

    timespec timeVal3;
    clock_gettime( CLOCK_REALTIME, &timeVal3 );

    uint64_t uSum    = 0;
    uint64_t uSumSq  = 0;
    for ( uns16* pPixel = pFrameBuffer; pPixel < (uns16*) ( (char*) pFrameBuffer + uFrameSize ); pPixel++ )
    {
      uSum += *pPixel;
      uSumSq += ((uint32_t)*pPixel) * ((uint32_t)*pPixel);
    }

    uint64_t uNumPixels = (int) (uFrameSize / sizeof(uns16));
    double   fAvgVal    = (double) uSum / (double) uNumPixels;
    double   fStdVal    = sqrt( (uNumPixels * uSumSq - uSum * uSum) / (double)(uNumPixels*uNumPixels));

    double fStartupTime = (timeVal2.tv_nsec - timeVal1.tv_nsec) * 1.e-6 + ( timeVal2.tv_sec - timeVal1.tv_sec ) * 1.e3;
    double fPollingTime = (timeVal3.tv_nsec - timeVal2.tv_nsec) * 1.e-6 + ( timeVal3.tv_sec - timeVal2.tv_sec ) * 1.e3;
    double fSingleFrameTime = fStartupTime + fPollingTime;
    if (fMaxSingleFrameTime < fSingleFrameTime)
      fMaxSingleFrameTime = fSingleFrameTime;

    printf("Startup Time = %6.1lf Polling Time = %7.1lf Frame Time = %.1lf ms Avg Val = %.2lf Std = %.2lf\n",
      fStartupTime, fPollingTime, fSingleFrameTime, fAvgVal, fStdVal );


    if ( sFnPrefix != NULL && sFnPrefix[0] != 0 )
    {
      char sFnOut[128];
      sprintf( sFnOut, "%s%02d.raw", sFnPrefix, iFrame );
      printf( "Outputting to %s...\n", sFnOut );
      FILE* fOut = fopen( sFnOut, "wb" );
      fwrite( pFrameBuffer, uFrameSize, 1, fOut );
      fclose(fOut);
    }

    if ( iFrame != 0 )
    {
      fAvgStartupTime += fStartupTime; fAvgPollingTime += fPollingTime;
      fAvgSingleFrameTime   += fSingleFrameTime;
      fSumSingleFrameTimeSq += fSingleFrameTime * fSingleFrameTime;
      fAvgAvgVal      += fAvgVal;
      fAvgStdVal      += fStdVal;
    }
  }

  // pl_exp_finish_seq(_hCam, pFrameBuffer, 0); // No need to call this function, unless we have multiple ROIs

  /*Uninit the sequence */
  pl_exp_uninit_seq();

  printf("Capture Init time = %6.1lf ms\n", fInitTime);
  if ( iNumFrame > 1 )
  {
    --iNumFrame;
    fAvgStartupTime /= iNumFrame; fAvgPollingTime     /= iNumFrame;

    fAvgAvgVal      /= iNumFrame; fAvgStdVal          /= iNumFrame;

    fAvgSingleFrameTime /= iNumFrame;
    double  fStdSingleFrameTime  = sqrt( fSumSingleFrameTimeSq / iNumFrame - fAvgSingleFrameTime * fAvgSingleFrameTime );
    printf("Average Startup Time = %6.1lf Polling Time = %7.1lf Avg Val = %.2lf Std = %.2lf\n",
      fAvgStartupTime, fAvgPollingTime, fAvgAvgVal, fAvgStdVal );
    printf("Frame Time Max %.1lf Avg %.1lf Std %.1lf ms\n", fMaxSingleFrameTime, fAvgSingleFrameTime, fStdSingleFrameTime );
  }

  free(pFrameBuffer);

  return 0;
}

/* Circ Buff and App Buff
   to be able to store more frames than the circular buffer can hold */
int ImageCapture::testImageCaptureContinous(int iNumFrame, int16 modeExposure, uns32 uExposureTime)
{
  printf( "Starting continuous image capture for %d frames, exposure mode = %d, exposure time = %d ms\n",
    iNumFrame, (int) modeExposure, (int) uExposureTime );

  const int16 iCircularBufferSize = 5;
  const timeval timeSleepMicroOrg = {0, 1000}; // 1 milliseconds

  rgn_type region;
  setupROI(region);
  PICAM::printROI(1, &region);

  /* Init a sequence set the region, exposure mode and exposure time */
  if (!pl_exp_init_seq())
  {
    printPvError("ImageCapture::testImageCaptureContinous(): pl_exp_init_seq() failed!\n");
    return 2;
  }

  uns32 uFrameSize = 0;
  //if( pl_exp_setup_cont( _hCam, 1, region, TIMED_MODE, exp_time, &uFrameSize, CIRC_NO_OVERWRITE ) ) {
  if (!pl_exp_setup_cont(_hCam, 1, &region, modeExposure, uExposureTime, &uFrameSize, CIRC_NO_OVERWRITE))
  {
    printPvError("experiment setup failed!\n");
    return 3;
  }
  printf( "frame size for continous capture = %lu\n", uFrameSize );

  /* set up a circular buffer */
  const int iHeaderSize = 1024 * 2; // Default to use 2k header size
  if ( (int) uFrameSize < iHeaderSize )
  {
    printf("ImageCapture::testImageCaptureContinous(): Frame(ROI) size (%lu bytes) is too small to fit in a DAQ packet header (%d bytes)\n",
     uFrameSize, iHeaderSize );
    return 4;
  }

  int iBufferSize = uFrameSize * iCircularBufferSize;
  unsigned char* pBufferWithHeader = (unsigned char *) malloc(iHeaderSize + iBufferSize);
  if (!pBufferWithHeader)
  {
    printf("ImageCapture::testImageCaptureContinous(): Memory allocation error!\n");
    return PV_FAIL;
  }
  uns16 *pFrameBuffer = (uns16*) (pBufferWithHeader + iHeaderSize);
  printf( "Continuous Frame Buffer starting from %p...\n", pFrameBuffer );

  /* Start the acquisition */
  printf("Collecting %i Frames\n", iNumFrame);
  if (!pl_exp_start_cont(_hCam, pFrameBuffer, iBufferSize))
  {
    printPvError("ImageCapture::testImageCaptureContinous():pl_exp_start_cont() failed");
    free(pBufferWithHeader);
    return PV_FAIL;
  }

  double fAvgPollingTime = 0, fAvgFrameProcessingTime = 0, fAvgReadoutTime = 0, fAvgSingleFrameTime = 0;
  /* ACQUISITION LOOP */
  for (int iFrame = 0; iFrame < iNumFrame; iFrame++)
  {
    if (giExitAll != 0)
    {
      iNumFrame = iFrame;
      break;
    }

    printf( "Taking frame %d...", iFrame );

    timespec timeVal1;
    clock_gettime( CLOCK_REALTIME, &timeVal1 );

    uns32 uNumBytesTransfered = -1, uNumBufferFilled = -1;
    int iNumLoop = 0;

    bool bInnerLoopError = false;
    /* wait for data or error */
    while (1)
    {
      int16 status = 0;
      if (!pl_exp_check_cont_status(_hCam, &status, &uNumBytesTransfered, &uNumBufferFilled) )
      {
        printPvError("ImageCapture::testImageCaptureContinous():pl_exp_start_cont() failed");
        bInnerLoopError = true;
        break;
      }

      /* Check Error Codes */
      if (status == READOUT_FAILED)
      {
        printf("testImageCaptureContinous():pl_exp_check_cont_status() return status=READOUT_FAILED\n");
        bInnerLoopError = true;
        break;
      }

      if (status == READOUT_COMPLETE)
        break;

      // This data will be modified by select(), so need to be reset
      timeval timeSleepMicro = timeSleepMicroOrg;
      // use select() to simulate nanosleep(), because experimentally select() controls the sleeping time more precisely
      select( 0, NULL, NULL, NULL, &timeSleepMicro);
      iNumLoop++;
    }

    if (bInnerLoopError) break;

    timespec timeVal2;
    clock_gettime( CLOCK_REALTIME, &timeVal2 );

    void* pFrameCurrent;
    if (!pl_exp_get_oldest_frame(_hCam, &pFrameCurrent))
    {
      printPvError("ImageCapture::testImageCaptureContinous():pl_exp_start_cont() failed");
      break;
    }

    printf( "Current frame = %p\n", pFrameCurrent );

    for ( int iCopy = 0; iCopy < 1; iCopy++ )
    {
      memcpy( pBufferWithHeader+iHeaderSize/2, pBufferWithHeader, iHeaderSize/2 );
    }

    if ( !pl_exp_unlock_oldest_frame(_hCam) )
    {
      printPvError("ImageCapture::testImageCaptureContinous():pl_exp_unlock_oldest_frame() failed");
      break;
    }

    timespec timeVal3;
    clock_gettime( CLOCK_REALTIME, &timeVal3 );


    double fReadoutTime = -1;
    PICAM::getAnyParam(_hCam, PARAM_READOUT_TIME, &fReadoutTime);

    double fPollingTime = (timeVal2.tv_nsec - timeVal1.tv_nsec) * 1.e-6 + ( timeVal2.tv_sec - timeVal1.tv_sec ) * 1.e3;
    double fFrameProcessingTime = (timeVal3.tv_nsec - timeVal2.tv_nsec) * 1.e-6 + ( timeVal3.tv_sec - timeVal2.tv_sec ) * 1.e3;
    double fSingleFrameTime = fPollingTime + fFrameProcessingTime ;
    printf(" Polling Time = %7.1lf Frame Processing Time = %.1lf Readout Time = %.1lf Frame Time = %.1lf\n",
      fPollingTime, fFrameProcessingTime, fReadoutTime, fSingleFrameTime );

    fAvgPollingTime += fPollingTime; fAvgFrameProcessingTime += fFrameProcessingTime; fAvgReadoutTime += fReadoutTime; fAvgSingleFrameTime += fSingleFrameTime;
  }

  /* Stop the acquisition */
  if (!pl_exp_stop_cont(_hCam, CCS_HALT)) printPvError("ImageCapture::testImageCaptureContinous():pl_exp_stop_cont() failed");

  // pl_exp_finish_seq(_hCam, pFrameBuffer, 0); // No need to call this function, unless we have multiple ROIs

  /* Uninit the sequence */
  if (!pl_exp_uninit_seq()) printPvError("ImageCapture::testImageCaptureContinous():pl_exp_uninit_seq() failed");

  free(pBufferWithHeader);

  if ( iNumFrame > 0 )
  {
    fAvgPollingTime /= iNumFrame; fAvgFrameProcessingTime /= iNumFrame; fAvgReadoutTime /= iNumFrame; fAvgSingleFrameTime /= iNumFrame;
    printf("Averge Polling Time = %7.1lf Frame Processing Time = %.1lf Readout Time = %.1lf Frame Time = %.1lf\n",
        fAvgPollingTime, fAvgFrameProcessingTime, fAvgReadoutTime, fAvgSingleFrameTime );
  }

  return PV_OK;
}


int ImageCapture::printStatus()
{
  uns16 u16DetectorWidth      = -1;
  uns16 u16DetectorHeight     = -1;
  int16 i16MaxSpeedTableIndex = -1;
  int16 i16SpeedTableIndex    = -1;
  int16 i16MaxGainIndex       = -1;
  int16 i16GainIndex          = -1;
  uns32 u32MaxReadoutPort     = 0;
  uns32 u32ReadoutPort        = 0;

  int16 i16TemperatureCurrent = -1;
  PICAM::getAnyParam(_hCam, PARAM_SER_SIZE, &u16DetectorWidth );
  PICAM::getAnyParam(_hCam, PARAM_PAR_SIZE, &u16DetectorHeight );
  PICAM::getAnyParam(_hCam, PARAM_SPDTAB_INDEX, &i16MaxSpeedTableIndex, ATTR_MAX);
  PICAM::getAnyParam(_hCam, PARAM_SPDTAB_INDEX, &i16SpeedTableIndex);
  PICAM::getAnyParam(_hCam, PARAM_GAIN_INDEX, &i16MaxGainIndex, ATTR_MAX, 0);
  PICAM::getAnyParam(_hCam, PARAM_GAIN_INDEX, &i16GainIndex, ATTR_CURRENT, 0);
  PICAM::getAnyParam(_hCam, PARAM_READOUT_PORT, &u32MaxReadoutPort, ATTR_MAX);
  PICAM::getAnyParam(_hCam, PARAM_READOUT_PORT, &u32ReadoutPort);
  PICAM::getAnyParam(_hCam, PARAM_TEMP, &i16TemperatureCurrent );
  printf( "Detector Width %d Height %d Speed %d/%d Gain %d/%d Port %d/%d Temperature %.1f C\n", u16DetectorWidth, u16DetectorHeight,
    i16SpeedTableIndex, i16MaxSpeedTableIndex, i16GainIndex, i16MaxGainIndex,
    (int) u32ReadoutPort, (int) u32MaxReadoutPort, i16TemperatureCurrent/100.f );

  return 0;
}
