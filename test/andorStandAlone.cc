#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <math.h>
#include <errno.h>
#include <getopt.h>
#include <pthread.h> 
#include <unistd.h>
#include <signal.h>
#include <iostream>
#include <fstream>
#include "andor/include/atmcdLXd.h"
#include "pds/andor/AndorErrorCodes.hh"

using namespace std;

static const char sAndorCameraTestVersion[] = "1.00";
int closeCamera();

static int iCameraInitialized = 0;
static bool isAndorFuncOk(int iError)
{
  return (iError == DRV_SUCCESS);
}

class AndorCameraTest
{
public:  
  AndorCameraTest(int iCamera, double fExposureTime, int iReadoutPort, int iSpeedIndex, int iGainIndex, 
    double fTemperature, int iRoiX, int iRoiY, int iRoiW, int iRoiH, int iBinX, int iBinY,
    char* sFnPrefix, int iNumImages);
    
  int init();
  int run();
  int deinit();
  
private:
  int _selectCamera(int iCamera);
  int _printCaps(AndorCapabilities &caps);
    
  int     _iCamera;
  double  _fExposureTime;
  int     _iReadoutPort;
  int     _iSpeedIndex;
  int     _iGainIndex;  
  double  _fTemperature;
  int     _iRoiX;
  int     _iRoiY;
  int     _iRoiW;
  int     _iRoiH;
  int     _iBinX;
  int     _iBinY;
  string  _strFnPrefix;
  int     _iNumImages;
  at_32   _iCameraHandle;    
  
  // Class usage control: Value semantics is disabled
  AndorCameraTest(const AndorCameraTest &);
  AndorCameraTest & operator=(const AndorCameraTest &);
};

AndorCameraTest::AndorCameraTest(int iCamera, double fExposureTime, int iReadoutPort, int iSpeedIndex, 
  int iGainIndex, double fTemperature, int iRoiX, int iRoiY, int iRoiW, int iRoiH, int iBinX, int iBinY,
  char* sFnPrefix, int iNumImages) :
  _iCamera(iCamera), _fExposureTime(fExposureTime), _iReadoutPort(iReadoutPort), _iSpeedIndex(iSpeedIndex), 
  _iGainIndex(iGainIndex), _fTemperature(fTemperature), _iRoiX(iRoiX), _iRoiY(iRoiY), _iRoiW(iRoiW), 
  _iRoiH(iRoiH), _iBinX(iBinX), _iBinY(iBinY), _strFnPrefix(sFnPrefix), _iNumImages(iNumImages),
  _iCameraHandle(0)
{
}

int AndorCameraTest::init()
{
  unsigned iError;  
  char sVersionInfo[128];
  iError = GetVersionInfo(AT_SDKVersion, sVersionInfo, sizeof(sVersionInfo));    
  if (isAndorFuncOk(iError))
    printf("SDKVersion: %s\n", sVersionInfo);
  else
    printf("GetVersionInfo(AT_SDKVersion): %s\n", AndorErrorCodes::name(iError));
      
  iError = _selectCamera(_iCamera);
  if (iError != 0)
    return 1;
    
  iError = Initialize("/usr/local/etc/andor");
  if (isAndorFuncOk(iError))
  {
    printf("Wait for hardware to finish initializing...\n");
    sleep(2); //sleep to allow initialization to complete
    iCameraInitialized = 1;
  }
  else
  {
    printf("Initialize(): %s\n", AndorErrorCodes::name(iError));  
    return 2;
  }
  
  iError = GetVersionInfo(AT_DeviceDriverVersion, sVersionInfo, sizeof(sVersionInfo));  
  if (isAndorFuncOk(iError))
    printf("DeviceDriverVersion: %s\n", sVersionInfo);
  else
    printf("GetVersionInfo(AT_DeviceDriverVersion): %s\n", AndorErrorCodes::name(iError));  
    
  unsigned int eprom    = 0;
  unsigned int coffile  = 0;
  unsigned int vxdrev   = 0;
  unsigned int vxdver   = 0;
  unsigned int dllrev   = 0;
  unsigned int dllver   = 0;
  iError = GetSoftwareVersion(&eprom, &coffile, &vxdrev, &vxdver, &dllrev, &dllver);    
  if (isAndorFuncOk(iError))
    printf("Software Version: eprom %d coffile %d vxdrev %d vxdver %d dllrev %d dllver %d\n",
      eprom, coffile, vxdrev, vxdver, dllrev, dllver);
  else
    printf("GetSoftwareVersion(): %s\n", AndorErrorCodes::name(iError));  
    
  unsigned int iPCB     = 0;
  unsigned int iDecode  = 0;
  unsigned int iDummy1  = 0;
  unsigned int iDummy2  = 0;
  unsigned int iCameraFirmwareVersion = 0;
  unsigned int iCameraFirmwareBuild   = 0;
  iError = GetHardwareVersion(&iPCB, &iDecode, &iDummy1, &iDummy2, &iCameraFirmwareVersion, &iCameraFirmwareBuild);
  if (isAndorFuncOk(iError))
    printf("Hardware Version: PCB %d Decode %d FirewareVer %d FirewareBuild %d\n",
      iPCB, iDecode, iCameraFirmwareVersion, iCameraFirmwareBuild);
  else
    printf("GetHardwareVersion(): %s\n", AndorErrorCodes::name(iError));  
  
    
  int iSerialNumber = -1;
  GetCameraSerialNumber(&iSerialNumber);
  if (isAndorFuncOk(iError))
    printf("Camera serial number: %d\n", iSerialNumber);
  else
    printf("GetCameraSerialNumber(): %s\n", AndorErrorCodes::name(iError));  
    
  char sHeadModel[256];
  GetHeadModel(sHeadModel);
  if (isAndorFuncOk(iError))
    printf("Camera Head Model: %s\n", sHeadModel);
  else
    printf("GetHeadModel(): %s\n", AndorErrorCodes::name(iError));  
  
  AndorCapabilities caps;
  iError = GetCapabilities(&caps);
  if (isAndorFuncOk(iError))
    _printCaps(caps);
  else
    printf("GetCapabilities(): %s\n", AndorErrorCodes::name(iError));  
  
  
  printf("init okay\n");
  return 0;
}

int AndorCameraTest::_selectCamera(int iCamera)
{
  at_32 iNumCameras;
  GetAvailableCameras(&iNumCameras);
  
  printf("Found %d Andor Cameras\n", (int) iNumCameras);
  
  if (iCamera < 0 || iCamera > iNumCameras)
  {
    printf("Invalid Camera selection: %d\n", iCamera);
    return 1;
  }
  
  GetCameraHandle(iCamera, &_iCameraHandle);
  SetCurrentCamera(_iCameraHandle);
  
  return 0;
}

int AndorCameraTest::run()
{
  //Set Read Mode to --Image--
  SetReadMode(4);

  //Set Acquisition mode to --Single scan--
  SetAcquisitionMode(1);

  //Set initial exposure time
  SetExposureTime((float)_fExposureTime);

  int width, height;
  //Get Detector dimensions
  GetDetector(&width, &height);
  printf("Detector width %d height %d\n", width, height);

  //Initialize Shutter
  SetShutter(1,0,0,0);
        
  //Setup Image dimensions
  SetImage(1,1,1,width,1,height);
    
  float fKeepCleanTime = -1;
  GetKeepCleanTime(&fKeepCleanTime);
  printf("Keep clean time: %f s\n", fKeepCleanTime);

  bool quit = false;
  do{
    //Show menu options
    cout << "        Menu" << endl;
    cout << "====================" << endl;
    cout << "a. Start Acquisition" << endl;
    cout << "b. Set Exposure Time" << endl;
    cout << "z.     Exit" << endl;
    cout << "====================" << endl;
    cout << "Choice?::";
    //Get menu choice
    int choice = getchar();

    switch(choice){
    case 'a': //Acquire
      {
      StartAcquisition();

      int status;
      at_32* imageData = new at_32[width*height];

      fstream fout("image.txt", ios::out);

      //Loop until acquisition finished
      GetStatus(&status);
      while(status==DRV_ACQUIRING) GetStatus(&status);

      GetAcquiredData(imageData, width*height);

      for(int i=0;i<width*height;i++) fout << imageData[i] << endl;

      SaveAsBmp("./image.bmp", "./GREY.PAL", 0, 0);

                          delete[] imageData;
      }

      break;
    
    case 'b': //Set new exposure time
      double fChoice;
      cout << endl << "Enter new Exposure Time(s)::";
      cin >> fChoice;

      SetExposureTime( (float) fChoice);

      break;

    case 'z': //Exit

      quit = true;

      break;
    
    default:

      cout << "!Invalid Option!" << endl;

    } 
    getchar();

  }while(!quit);  
  
  return 0;
}

int AndorCameraTest::deinit()
{
  closeCamera();
  
  return 0;
}

int AndorCameraTest::_printCaps(AndorCapabilities &caps)
{
  printf("Capabilities:\n");
  printf("  Size              : %d\n",   (int) caps.ulSize);
  printf("  AcqModes          : 0x%x\n", (int) caps.ulAcqModes);  
  printf("    AC_ACQMODE_SINGLE         : %d\n", (caps.ulAcqModes & AC_ACQMODE_SINGLE)? 1:0 );  
  printf("    AC_ACQMODE_VIDEO          : %d\n", (caps.ulAcqModes & AC_ACQMODE_VIDEO)? 1:0 );  
  printf("    AC_ACQMODE_ACCUMULATE     : %d\n", (caps.ulAcqModes & AC_ACQMODE_ACCUMULATE)? 1:0 );  
  printf("    AC_ACQMODE_KINETIC        : %d\n", (caps.ulAcqModes & AC_ACQMODE_KINETIC)? 1:0 );  
  printf("    AC_ACQMODE_FRAMETRANSFER  : %d\n", (caps.ulAcqModes & AC_ACQMODE_FRAMETRANSFER)? 1:0 );  
  printf("    AC_ACQMODE_FASTKINETICS   : %d\n", (caps.ulAcqModes & AC_ACQMODE_FASTKINETICS)? 1:0 );  
  printf("    AC_ACQMODE_OVERLAP  : %d\n", (caps.ulAcqModes & AC_ACQMODE_OVERLAP)? 1:0 );  
    
  printf("  ReadModes         : 0x%x\n", (int) caps.ulReadModes);  
    
  printf("  TriggerModes      : 0x%x\n", (int) caps.ulTriggerModes);    
  printf("    AC_TRIGGERMODE_INTERNAL         : %d\n", (caps.ulTriggerModes & AC_TRIGGERMODE_INTERNAL)? 1:0 );  
  printf("    AC_TRIGGERMODE_EXTERNAL         : %d\n", (caps.ulTriggerModes & AC_TRIGGERMODE_EXTERNAL)? 1:0 );  
  printf("    AC_TRIGGERMODE_EXTERNAL_FVB_EM  : %d\n", (caps.ulTriggerModes & AC_TRIGGERMODE_EXTERNAL_FVB_EM)? 1:0 );  
  printf("    AC_TRIGGERMODE_CONTINUOUS       : %d\n", (caps.ulTriggerModes & AC_TRIGGERMODE_CONTINUOUS)? 1:0 );  
  printf("    AC_TRIGGERMODE_EXTERNALSTART    : %d\n", (caps.ulTriggerModes & AC_TRIGGERMODE_EXTERNALSTART)? 1:0 );  
  printf("    AC_TRIGGERMODE_EXTERNALEXPOSURE : %d\n", (caps.ulTriggerModes & AC_TRIGGERMODE_EXTERNALEXPOSURE)? 1:0 );  
  
  printf("  CameraType        : 0x%x\n", (int) caps.ulCameraType);
  printf("    AC_CAMERATYPE_IKON  : %d\n", (caps.ulCameraType == AC_CAMERATYPE_IKON)? 1:0 );  
  
  printf("  PixelMode         : 0x%x\n", (int) caps.ulPixelMode);
  printf("  SetFunctions      : 0x%x\n", (int) caps.ulSetFunctions);  
  printf("  GetFunctions      : 0x%x\n", (int) caps.ulGetFunctions);  
  printf("  Features          : 0x%x\n", (int) caps.ulFeatures);  
  printf("  PCICard           : 0x%x\n", (int) caps.ulPCICard);  
  printf("  EMGainCapability  : 0x%x\n", (int) caps.ulEMGainCapability);  
  printf("  FTReadModes       : 0x%x\n", (int) caps.ulFTReadModes);  
  return 0;
}

int closeCamera()
{
  AbortAcquisition();
  
  int temp = -999;
  GetTemperature(&temp);
  if(temp!=-999 && temp<5)
    cout << "Wait until temperature rises above 5C, before exiting" << endl;
  CoolerOFF();  
  
  if (iCameraInitialized != 0)
  {
    ShutDown();    
    iCameraInitialized = 0;
  }
  return 0;
}


static void showUsage()
{
    printf( "Usage:  andorStandAlone  [-v|--version] [-h|--help] [-c|--camera <camera number>]\n"
      "[-w|--write <filename prefix>] [-n|--number <number of images>] [-e|--exposure <exposure time (sec)>]\n"
      "[-p|--port <readout port>] [-s|--speed <speed index>] [-g|--gain <gain index>]\n"
      "[-t|--temp <temperature>] [-r|--roi <x,y,w,h>] [-b|--bin <xbin,ybin>]\n"
      "  Options:\n"
      "    -v|--version                      Show file version\n"
      "    -h|--help                         Show usage\n"
      "    -c|--camera    <camera number>    Select camera\n"
      "    -w|--write     <filename prefix>  Output filename prefix\n"
      "    -n|--number    <number of images> Number of images to be captured (Default: 1)\n"
      "    -e|--exposure  <exposure time>    Exposure time (sec) (Default: 1 sec)\n"      
      "    -p|--port      <readout port>     Readout port\n"      
      "    -s|--speed     <speed inedx>      Speed table index\n"      
      "    -g|--gain      <gain index>       Gain Index\n"      
      "    -t|--temp      <temperature>      Temperature (in Celcius) (Default: 25 C)\n"
      "    -r|--roi       <x,y,w,h>          Region of Interest\n"
      "    -b|--bin       <xbin,ybin>        Binning of X/Y\n"
    );
}

static void showVersion()
{
    printf( "Version:  andorStandAlone  Ver %s\n", sAndorCameraTestVersion );
}

static int giExitAll = 0;
void signalIntHandler(int iSignalNo)
{
  printf( "\nsignalIntHandler(): signal %d received. Stopping all activities\n", iSignalNo );  
  closeCamera();  
  giExitAll = 1;   
  exit(1);
}

int main(int argc, char **argv)
{
  const char*         strOptions  = ":vhc:w:n:e:p:s:g:t:r:b:";
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
     {0,          0, 0,  0 }
  };    
  
  int     iCamera        = 0;
  char    sFnPrefix[32] = "";
  int     iNumImages    = 1;
  double  fExposureTime = 1.0;
  int     iReadoutPort  = -1;
  int     iSpeedIndex   = -1;
  int     iGainIndex    = -1;
  float   fTemperature  = 25.0f;
  int     iRoiX         = 0;
  int     iRoiY         = 0;
  int     iRoiW         = -1;
  int     iRoiH         = -1;
  int     iBinX         = 1;
  int     iBinY         = 1;
  
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
          fExposureTime = strtod(optarg, NULL);
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
      case '?':               /* Terse output mode */
          printf( "andorStandAlone:main(): Unknown option: %c\n", optopt );
          break;
      case ':':               /* Terse output mode */
          printf( "andorStandAlone:main(): Missing argument for %c\n", optopt );
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

  AndorCameraTest testCamera(iCamera, fExposureTime, iReadoutPort, iSpeedIndex, iGainIndex, 
    fTemperature, iRoiX, iRoiY, iRoiW, iRoiH, iBinX, iBinY, sFnPrefix, iNumImages);
    
  int iError = testCamera.init();
  if (iError == 0)
  {
    iError = testCamera.run();    
  }
  
  testCamera.deinit();  
  return 0;
}
