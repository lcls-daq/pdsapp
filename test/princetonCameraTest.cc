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
#include "pvcam/include/master.h"
#include "pvcam/include/pvcam.h"

int testPIDaq(int iCamera, char* sFnPrefix, int iNumImages, int iExposureTime, int iReadoutPort, int iSpeedIndex, 
  int iGainIndex, float fTemperature, int iStrip, int iRoiX, int iRoiY, int iRoiW, int iRoiH, int iBinX, int iBinY,
  int iMenu);

static const char sPrincetonCameraTestVersion[] = "1.40";

namespace PICAM
{
  void printPvError(const char *sPrefixMsg);
  void displayParamIdInfo(int16 hCam, uns32 uParamId,
                          const char sDescription[]);
  int getAnyParam(int16 hCam, uns32 uParamId, void *pParamValue, int16 iMode = ATTR_CURRENT, int iErrorReport = 1);
  int setAnyParam(int16 hCam, uns32 uParamId, void *pParamValue);
  
  void printROI(int iNumRoi, rgn_type* roi);
}

using PICAM::printPvError;

class ImageCapture
{
public:
  
  int   start(int iCamera, char* sFnPrefix, int iNumImages, int iExposureTime, int iReadoutPort, int iSpeedIndex, 
    int iGainIndex, float fTemperature, int iStrip, int iRoiX, int iRoiY, int iRoiW, int iRoiH, int iBinX, int iBinY, int iMenu);
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
  static int updateCameraSettings(int iReadoutPort, int iSpeedIndex, int iGainIndex, int iStrip);
  static int setupCooling(float fTemperature);  
  static int printStatus();
  
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
      "    [-m|--menu] [--strip <strip cycle>]\n"
      "  Options:\n"
      "    -v|--version                      Show file version\n"
      "    -h|--help                         Show usage\n"
      "    -c|--camera    <camera number>    Select camera\n"
      "    -w|--write     <filename prefix>  Output filename prefix\n"
      "    -n|--number    <number of images> Number of images to be captured (Default: 1)\n"
      "    -e|--exposure  <exposure time>    Exposure time (in ms) (Default: 1 ms)\n"      
      "    -p|--port      <readout port>     Readout port\n"      
      "    -s|--speed     <speed inedx>      Speed table index\n"      
      "    -g|--gain      <gain index>       Gain Index\n"      
      "    -t|--temp      <temperature>      Temperature (in Celcius) (Default: 25 C)\n"            
      "    -r|--roi       <x,y,w,h>          Region of Interest\n"
      "    -b|--bin       <xbin,ybin>        Binning of X/Y\n"
      "    -m|--menu                         Show interactive menu\n"
      "       --strip     <strip cycle>      Number of rows per clear cycle\n"
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
     {"strip",    1, 0,  1000},     
     {0,          0, 0,  0 }
  };    
  
  int   iCamera        = 0;
  char  sFnPrefix[32] = "";
  int   iNumImages    = 1;
  int   iExposureTime = 1;
  int   iReadoutPort  = -1;
  int   iSpeedIndex   = -1;
  int   iGainIndex    = -1;
  float fTemperature  = 25.0f;
  int   iStrip        = -1;
  int   iRoiX         = 0;
  int   iRoiY         = 0;
  int   iRoiW         = -1;
  int   iRoiH         = -1;
  int   iBinX         = 1;
  int   iBinY         = 1;  
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
          iStrip        = strtoul(optarg, NULL, 0);
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

  testPIDaq(iCamera, sFnPrefix, iNumImages, iExposureTime, iReadoutPort, iSpeedIndex, iGainIndex, fTemperature, iStrip, iRoiX, iRoiY, iRoiW, iRoiH, iBinX, iBinY, iMenu);
}

int testPIDaq(int iCamera, char* sFnPrefix, int iNumImages, int iExposureTime, int iReadoutPort, int iSpeedIndex, 
  int iGainIndex, float fTemperature, int iStrip, int iRoiX, int iRoiY, int iRoiW, int iRoiH, int iBinX, int iBinY, 
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
    iGainIndex, fTemperature, iStrip, iRoiX, iRoiY, iRoiW, iRoiH, iBinX, iBinY, iMenu);
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

int ImageCapture::updateCameraSettings(int iReadoutPort, int iSpeedIndex, int iGainIndex, int iStrip)
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

using namespace std;

int ImageCapture::start(int iCamera, char* sFnPrefix, int iNumImages, int iExposureTime, int iReadoutPort, int iSpeedIndex, 
  int iGainIndex, float fTemperature, int iStrip, int iRoiX, int iRoiY, int iRoiW, int iRoiH, int iBinX, int iBinY, int iMenu)
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
    
  int iWidth, iHeight;
  PICAM::getAnyParam( _hCam, PARAM_SER_SIZE, &iWidth );
  PICAM::getAnyParam( _hCam, PARAM_PAR_SIZE, &iHeight );    
  if (iRoiW < 0 )
    iRoiW = iWidth;
  if (iRoiH < 0 )
    iRoiH = iHeight;

  printStatus();
    
  setupCooling(fTemperature);

  updateCameraSettings(iReadoutPort, iSpeedIndex, iGainIndex, iStrip);

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
        cout << "Output Filename Prefix  : " << strFnPrefix   << endl;
        cout << "Number Images per Acq   : " << iNumImages    << endl;
        cout << "Exposure Time (sec)     : " << fExposureTime << endl;
        cout << "Readout Port            : " << iReadoutPort  << endl;
        cout << "Speed Index             : " << iSpeedIndex   <<  endl;
        cout << "Gain Index              : " << iGainIndex    << endl;
        cout << "Cooling Temperature (C) : " << fTemperature  << endl;
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
        cout << "t. Set Cooling Temperature" << endl;
        cout << "r. Set ROI" << endl;
        cout << "b. Set Binning" << endl;    
        cout << "q. Quit Program" << endl;
        cout << "====================================="  << endl;
        cout << "> ";
        //Get menu choice
        int choice = getchar();

        switch(choice){
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
          updateCameraSettings(iReadoutPort, iSpeedIndex, iGainIndex, iStrip);
          break;
        }    
        case 's': 
        {
          cout << endl << "Enter new Speed Index > ";
          cin >> iSpeedIndex;
          printf("New Speed Index: %d\n", iSpeedIndex);
          updateCameraSettings(iReadoutPort, iSpeedIndex, iGainIndex, iStrip);
          break;
        }    
        case 'g': 
        {
          cout << endl << "Enter new Gain Index > ";
          cin >> iGainIndex;
          printf("New Gain Index: %d\n", iGainIndex);
          updateCameraSettings(iReadoutPort, iSpeedIndex, iGainIndex, iStrip);
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
      }
      while (!bQuitMenu);
    } // if (iMenu != 0)
    
    if (bQuitCapture) 
      break;
  
    const int iNumFrame         = iNumImages;
    const int16 modeExposure    = TIMED_MODE;
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
  int iWidth, iHeight;
  getAnyParam( _hCam, PARAM_SER_SIZE, &iWidth );
  getAnyParam( _hCam, PARAM_PAR_SIZE, &iHeight );

  // set the region to use full frame and 1x1 binning
  region.s1 = 0;
  region.s2 = iWidth - 1;
  region.sbin = 1;
  region.p1 = 0;
  region.p2 = iHeight - 1;
  region.pbin = 1;

  return 0;
}

int ImageCapture::setupROI(rgn_type& region, int iRoiX, int iRoiY, int iRoiW, int iRoiH, int iBinX, int iBinY)
{
  using PICAM::getAnyParam;
  int iWidth, iHeight;
  getAnyParam( _hCam, PARAM_SER_SIZE, &iWidth );
  getAnyParam( _hCam, PARAM_PAR_SIZE, &iHeight );
  
  // set the region to use full frame and 1x1 binning
  region.s1 = iRoiX;
  region.s2 = iRoiX + iRoiW - 1;
  region.sbin = iBinX;
  region.p1 = iRoiY;
  region.p2 = iRoiY + iRoiH - 1;
  region.pbin = iBinY;
  
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
    printf("Average Startup Time = %6.1lf Polling Time = %7.1lf Frame Time = %.1lf std %.1lf ms Avg Val = %.2lf Std = %.2lf\n", 
      fAvgStartupTime, fAvgPollingTime, fAvgSingleFrameTime, fStdSingleFrameTime, fAvgAvgVal, fAvgStdVal );        
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
  int16 i16DetectorWidth      = -1;
  int16 i16DetectorHeight     = -1;  
  int16 i16MaxSpeedTableIndex = -1;
  int16 i16SpeedTableIndex    = -1;
  int16 i16MaxGainIndex       = -1;
  int16 i16GainIndex          = -1;
  uns32 u32MaxReadoutPort     = 0; 
  uns32 u32ReadoutPort        = 0; 
  
  int16 i16TemperatureCurrent = -1;  
  PICAM::getAnyParam(_hCam, PARAM_SER_SIZE, &i16DetectorWidth );
  PICAM::getAnyParam(_hCam, PARAM_PAR_SIZE, &i16DetectorHeight );
  PICAM::getAnyParam(_hCam, PARAM_SPDTAB_INDEX, &i16MaxSpeedTableIndex, ATTR_MAX);  
  PICAM::getAnyParam(_hCam, PARAM_SPDTAB_INDEX, &i16SpeedTableIndex);    
  PICAM::getAnyParam(_hCam, PARAM_GAIN_INDEX, &i16MaxGainIndex, ATTR_MAX, 0);  
  PICAM::getAnyParam(_hCam, PARAM_GAIN_INDEX, &i16GainIndex, ATTR_CURRENT, 0);  
  PICAM::getAnyParam(_hCam, PARAM_READOUT_PORT, &u32MaxReadoutPort, ATTR_MAX);    
  PICAM::getAnyParam(_hCam, PARAM_READOUT_PORT, &u32ReadoutPort);
  PICAM::getAnyParam(_hCam, PARAM_TEMP, &i16TemperatureCurrent );  
  printf( "Detector Width %d Height %d Speed %d/%d Gain %d/%d Port %d/%d Temperature %.1f C\n", i16DetectorWidth, i16DetectorHeight, 
    i16SpeedTableIndex, i16MaxSpeedTableIndex, i16GainIndex, i16MaxGainIndex, 
    (int) u32ReadoutPort, (int) u32MaxReadoutPort, i16TemperatureCurrent/100.f );   
    
  return 0;
}

namespace PICAM
{
  
char *lsParamAccessMode[] =
{ "ACC_ERROR", "ACC_READ_ONLY", "ACC_READ_WRITE", "ACC_EXIST_CHECK_ONLY", "ACC_WRITE_ONLY"
};

void printPvError(const char *sPrefixMsg)
{
  char sErrorMsg[ERROR_MSG_LEN];

  int16 iErrorCode = pl_error_code();
  pl_error_message(iErrorCode, sErrorMsg);
  printf("%s\n  PvCam library error code [%i] %s\n", sPrefixMsg,
         iErrorCode, sErrorMsg);
}

/* this routine assumes the param id is an enumerated type, it will print out all the */
/* enumerated values that are allowed with the param id and display the associated    */
/* ASCII text.                                                                        */
static void displayParamEnumInfo(int16 hCam, uns32 uParamId)
{
  boolean status;             /* status of pvcam functions.                */
  uns32 count, index;         /* counters for enumerated types.            */
  char enumStr[100];          /* string for enum text.                     */
  int32 enumValue;            /* enum value returned for index & param id. */
  uns32 uCurVal;

  status = pl_get_param(hCam, uParamId, ATTR_CURRENT, (void *) &uCurVal);
  if (status)
    printf(" current value = %lu\n", uCurVal);
  else
  {
    printf(" parameter %lu is not available\n", uParamId);
  }

  /* get number of enumerated values. */
  status = pl_get_param(hCam, uParamId, ATTR_COUNT, (void *) &count);
  printf(" count = %lu\n", count);
  if (status)
  {
    for (index = 0; index < count; index++)
    {
      /* get enum value and enum string */
      status = pl_get_enum_param(hCam, uParamId, index, &enumValue,
                                 enumStr, 100);
      /* if everything alright print out the results. */
      if (status)
      {
        printf(" [%ld] enum value = %ld, text = %s\n",
               index, enumValue, enumStr);
      }
      else
      {
        printf
          ("functions failed pl_get_enum_param, with error code %d\n",
           pl_error_code());
      }
    }
  }
  else
  {
    printf("functions failed pl_get_param, with error code %d\n",
           pl_error_code());
  }
}                             /* end of function DisplayEnumInfo */

static void displayParamValueInfo(int16 hCam, uns32 uParamId)
{
  /* current, min&max, & default values of parameter id */
  union
  {
    double dval;
    unsigned long ulval;
    long lval;
    unsigned short usval;
    short sval;
    unsigned char ubval;
    signed char bval;
  } currentVal, minVal, maxVal, defaultVal, incrementVal;
  uns16 type;                 /* data type of paramater id */
  boolean status = 0, status2 = 0, status3 = 0, status4 = 0, status5 = 0; /* status of pvcam functions */

  /* get the data type of parameter id */
  status = pl_get_param(hCam, uParamId, ATTR_TYPE, (void *) &type);
  /* get the default, current, min and max values for parameter id. */
  /* Note : since the data type for these depends on the parameter  */
  /* id you have to call pl_get_param with the correct data type    */
  /* passed for pParamValue.                                        */
  if (status)
  {
    switch (type)
    {
    case TYPE_INT8:
      status = pl_get_param(hCam, uParamId, ATTR_CURRENT,
                            (void *) &currentVal.bval);
      status2 = pl_get_param(hCam, uParamId, ATTR_DEFAULT,
                             (void *) &defaultVal.bval);
      status3 = pl_get_param(hCam, uParamId, ATTR_MAX,
                             (void *) &maxVal.bval);
      status4 = pl_get_param(hCam, uParamId, ATTR_MIN,
                             (void *) &minVal.bval);
      status5 = pl_get_param(hCam, uParamId, ATTR_INCREMENT,
                             (void *) &incrementVal.bval);
      printf(" current value = %c\n", currentVal.bval);
      printf(" default value = %c\n", defaultVal.bval);
      printf(" min = %c, max = %c\n", minVal.bval, maxVal.bval);
      printf(" increment = %c\n", incrementVal.bval);
      break;
    case TYPE_UNS8:
      status = pl_get_param(hCam, uParamId, ATTR_CURRENT,
                            (void *) &currentVal.ubval);
      status2 = pl_get_param(hCam, uParamId, ATTR_DEFAULT,
                             (void *) &defaultVal.ubval);
      status3 = pl_get_param(hCam, uParamId, ATTR_MAX,
                             (void *) &maxVal.ubval);
      status4 = pl_get_param(hCam, uParamId, ATTR_MIN,
                             (void *) &minVal.ubval);
      status5 = pl_get_param(hCam, uParamId, ATTR_INCREMENT,
                             (void *) &incrementVal.ubval);
      printf(" current value = %uc\n", currentVal.ubval);
      printf(" default value = %uc\n", defaultVal.ubval);
      printf(" min = %uc, max = %uc\n", minVal.ubval, maxVal.ubval);
      printf(" increment = %uc\n", incrementVal.ubval);
      break;
    case TYPE_INT16:
      status = pl_get_param(hCam, uParamId, ATTR_CURRENT,
                            (void *) &currentVal.sval);
      status2 = pl_get_param(hCam, uParamId, ATTR_DEFAULT,
                             (void *) &defaultVal.sval);
      status3 = pl_get_param(hCam, uParamId, ATTR_MAX,
                             (void *) &maxVal.sval);
      status4 = pl_get_param(hCam, uParamId, ATTR_MIN,
                             (void *) &minVal.sval);
      status5 = pl_get_param(hCam, uParamId, ATTR_INCREMENT,
                             (void *) &incrementVal.sval);
      printf(" current value = %i\n", currentVal.sval);
      printf(" default value = %i\n", defaultVal.sval);
      printf(" min = %i, max = %i\n", minVal.sval, maxVal.sval);
      printf(" increment = %i\n", incrementVal.sval);
      break;
    case TYPE_UNS16:
      status = pl_get_param(hCam, uParamId, ATTR_CURRENT,
                            (void *) &currentVal.usval);
      status2 = pl_get_param(hCam, uParamId, ATTR_DEFAULT,
                             (void *) &defaultVal.usval);
      status3 = pl_get_param(hCam, uParamId, ATTR_MAX,
                             (void *) &maxVal.usval);
      status4 = pl_get_param(hCam, uParamId, ATTR_MIN,
                             (void *) &minVal.usval);
      status5 = pl_get_param(hCam, uParamId, ATTR_INCREMENT,
                             (void *) &incrementVal.usval);
      printf(" current value = %u\n", currentVal.usval);
      printf(" default value = %u\n", defaultVal.usval);
      printf(" min = %u, max = %u\n", minVal.usval, maxVal.usval);
      printf(" increment = %u\n", incrementVal.usval);
      break;
    case TYPE_INT32:
      status = pl_get_param(hCam, uParamId, ATTR_CURRENT,
                            (void *) &currentVal.lval);
      status2 = pl_get_param(hCam, uParamId, ATTR_DEFAULT,
                             (void *) &defaultVal.lval);
      status3 = pl_get_param(hCam, uParamId, ATTR_MAX,
                             (void *) &maxVal.lval);
      status4 = pl_get_param(hCam, uParamId, ATTR_MIN,
                             (void *) &minVal.lval);
      status5 = pl_get_param(hCam, uParamId, ATTR_INCREMENT,
                             (void *) &incrementVal.lval);
      printf(" current value = %ld\n", currentVal.lval);
      printf(" default value = %ld\n", defaultVal.lval);
      printf(" min = %ld, max = %ld\n", minVal.lval, maxVal.lval);
      printf(" increment = %ld\n", incrementVal.lval);
      break;
    case TYPE_UNS32:
      status = pl_get_param(hCam, uParamId, ATTR_CURRENT,
                            (void *) &currentVal.ulval);
      status2 = pl_get_param(hCam, uParamId, ATTR_DEFAULT,
                             (void *) &defaultVal.ulval);
      status3 = pl_get_param(hCam, uParamId, ATTR_MAX,
                             (void *) &maxVal.ulval);
      status4 = pl_get_param(hCam, uParamId, ATTR_MIN,
                             (void *) &minVal.ulval);
      status5 = pl_get_param(hCam, uParamId, ATTR_INCREMENT,
                             (void *) &incrementVal.ulval);
      printf(" current value = %ld\n", currentVal.ulval);
      printf(" default value = %ld\n", defaultVal.ulval);
      printf(" min = %ld, max = %ld\n", minVal.ulval, maxVal.ulval);
      printf(" increment = %ld\n", incrementVal.ulval);
      break;
    case TYPE_FLT64:
      status = pl_get_param(hCam, uParamId, ATTR_CURRENT,
                            (void *) &currentVal.dval);
      status2 = pl_get_param(hCam, uParamId, ATTR_DEFAULT,
                             (void *) &defaultVal.dval);
      status3 = pl_get_param(hCam, uParamId, ATTR_MAX,
                             (void *) &maxVal.dval);
      status4 = pl_get_param(hCam, uParamId, ATTR_MIN,
                             (void *) &minVal.dval);
      status5 = pl_get_param(hCam, uParamId, ATTR_INCREMENT,
                             (void *) &incrementVal.dval);
      printf(" current value = %g\n", currentVal.dval);
      printf(" default value = %g\n", defaultVal.dval);
      printf(" min = %g, max = %g\n", minVal.dval, maxVal.dval);
      printf(" increment = %g\n", incrementVal.dval);
      break;
    case TYPE_BOOLEAN:
      status = pl_get_param(hCam, uParamId, ATTR_CURRENT,
                            (void *) &currentVal.bval);
      status2 = pl_get_param(hCam, uParamId, ATTR_DEFAULT,
                             (void *) &defaultVal.bval);
      printf(" current value = %d\n", (int) currentVal.bval);
      printf(" default value = %d\n", (int) defaultVal.bval);
      break;
    default:
      printf(" data type %d not supported in this functions\n", type );
      break;
    }
    if (!status || !status2 || !status3 || !status4 || !status5)
    {
      printf("functions failed pl_get_param, with error code %d\n",
             pl_error_code());
    }
  }
  else
  {
    printf("functions failed pl_get_param, with error code %d\n",
           pl_error_code());
  }
}

void displayParamIdInfo(int16 hCam, uns32 uParamId,
                        const char sDescription[])
{
  boolean status, status2;    /* status of pvcam functions                           */
  boolean avail_flag;         /* ATTR_AVAIL, is param id available                   */
  uns16 access = 0;           /* ATTR_ACCESS, get if param is read, write or exists. */
  uns16 type;                 /* ATTR_TYPE, get data type.                           */

  printf("%s (param id %lu):\n", sDescription, uParamId);
  status = pl_get_param(hCam, uParamId, ATTR_AVAIL, (void *) &avail_flag);
  /* check for errors */
  if (status)
  {
/* check to see if paramater id is supported by hardware, software or system.  */
    if (avail_flag)
    {
      /* we got a valid parameter, now get access writes and data type */
      status = pl_get_param(hCam, uParamId, ATTR_ACCESS, (void *) &access);
      status2 = pl_get_param(hCam, uParamId, ATTR_TYPE, (void *) &type);
      if (status && status2)
      {
        printf(" access mode = %s\n", lsParamAccessMode[access]);

        if (access == ACC_EXIST_CHECK_ONLY)
        {
          printf(" param id %lu exists\n", uParamId);
        }
        else if ((access == ACC_READ_ONLY) || (access == ACC_READ_WRITE))
        {
          /* now we can start displaying information. */
          /* handle enumerated types separate from other data */
          if (type == TYPE_ENUM)
          {
            displayParamEnumInfo(hCam, uParamId);
          }
          else
          {                   /* take care of he rest of the data types currently used */
            displayParamValueInfo(hCam, uParamId);
          }
        }
        else
        {
          printf(" error in access check for param id %lu\n", uParamId);
        }
      }
      else
      {                       /* error occured calling function. */
        printf
          ("functions failed pl_get_param, with error code %d\n",
           pl_error_code());
      }

    }
    else
    {                         /* parameter id is not available with current setup */
      printf
        (" parameter %ld is not available with current hardware or software setup\n",
         uParamId);
    }
  }
  else
  {                           /* error occured calling function print out error and error code */
    printf("functions failed pl_get_param, with error code %d\n",
           pl_error_code());
  }
}                             /* end of function displayParamIdInfo */

int getAnyParam(int16 hCam, uns32 uParamId, void *pParamValue, int16 iMode, int iErrorReport)
{
  if (pParamValue == NULL)
  {
    printf("getAnyParam(): Invalid parameter: pParamValue=%p\n", pParamValue );
    return 1;
  }

  rs_bool bStatus, bParamAvailable;
  uns16 uParamAccess;

  bStatus =
    pl_get_param(hCam, uParamId, ATTR_AVAIL, (void *) &bParamAvailable);
  if (!bStatus)
  {
    printf("getAnyParam(): pl_get_param(param id = %lu, ATTR_AVAIL) failed\n",
     uParamId);
    return 2;
  }
  else if (!bParamAvailable)
  {
    if (iErrorReport >= 1)
      printf("getAnyParam(): param id %lu is not available\n", uParamId);
    return 3;
  }

  bStatus = pl_get_param(hCam, uParamId, ATTR_ACCESS, (void *) &uParamAccess);
  if (!bStatus)
  {
    printf("getAnyParam(): pl_get_param(param id = %lu, ATTR_ACCESS) failed\n",
     uParamId);
    return 4;
  }
  else if (uParamAccess != ACC_READ_WRITE && uParamAccess != ACC_READ_ONLY)
  {
    printf("getAnyParam(): param id %lu is not writable, access mode = %s\n",
     uParamId, lsParamAccessMode[uParamAccess]);
    return 5;
  }

  bStatus = pl_get_param(hCam, uParamId, iMode, pParamValue);
  if (!bStatus)
  {
    printf("getAnyParam(): pl_get_param(param id = %lu, ATTR_CURRENT) failed\n", uParamId);
    return 6;
  }

  return 0;
}

int setAnyParam(int16 hCam, uns32 uParamId, void *pParamValue)
{
  rs_bool bStatus, bParamAvailable;
  uns16 uParamAccess;

  bStatus =
    pl_get_param(hCam, uParamId, ATTR_AVAIL, (void *) &bParamAvailable);
  if (!bStatus)
  {
    printf("setAnyParam(): pl_get_param(param id = %lu, ATTR_AVAIL) failed\n", uParamId);
    return 2;
  }
  else if (!bParamAvailable)
  {
    printf("setAnyParam(): param id %lu is not available\n", uParamId);
    return 3;
  }

  bStatus = pl_get_param(hCam, uParamId, ATTR_ACCESS, (void *) &uParamAccess);
  if (!bStatus)
  {
    printf("setAnyParam(): pl_get_param(param id = %lu, ATTR_ACCESS) failed\n", uParamId);
    return 4;
  }
  else if (uParamAccess != ACC_READ_WRITE && uParamAccess != ACC_WRITE_ONLY)
  {
    printf("setAnyParam(): param id %lu is not writable, access mode = %s\n",
     uParamId, lsParamAccessMode[uParamAccess]);
    return 5;
  }

  bStatus = pl_set_param(hCam, uParamId, pParamValue);
  if (!bStatus)
  {
    printf("setAnyParam(): pl_set_param(param id = %lu) failed\n", uParamId);
    return 6;
  }

  return 0;
}

void printROI(int iNumRoi, rgn_type* roi)
{
  for (int i = 0; i < iNumRoi; i++)
  {
    printf
      ("ROI %i set to { s1 = %i, s2 = %i, sin = %i, p1 = %i, p2 = %i, pbin = %i }\n",
       i, roi[i].s1, roi[i].s2, roi[i].sbin, roi[i].p1, roi[i].p2,
       roi[i].pbin);
  }
}

} // namespace PICAM

// To avoid pvcam.so crappy dependency on firewire raw1394 shared library
extern "C" {

void raw1394_arm_register() {}
void raw1394_new_handle_on_port() {}
void raw1394_new_handle() {}
void raw1394_get_nodecount() {}
void raw1394_arm_get_buf() {}
void raw1394_arm_unregister() {}
void raw1394_destroy_handle() {}
void raw1394_read() {}
void raw1394_write() {}
void raw1394_get_port_info() {}
void raw1394_get_fd() {}
void raw1394_loop_iterate() {}

}
