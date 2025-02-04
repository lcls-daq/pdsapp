#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <math.h>
#include <errno.h>
#include <getopt.h>
#include <pthread.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <iostream>
#include <fstream>
#include "andor/include/atmcdLXd.h"
#include "pds/andor/AndorErrorCodes.hh"
#include "pds/oceanoptics/histreport.hh"

using namespace std;

static const char sAndorCameraTestVersion[] = "1.21";
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
    double fTemperature, int iRoiX, int iRoiY, int iRoiW, int iRoiH, int iBinX, int iBinY, int iTriggerMode,
    char* sFnPrefix, int iNumImages, int iOutputMode, int iMenu, bool bTriggerFast, bool bCropMode);

  int init();
  int run();
  int deinit();

private:
  int _selectCamera(int iCamera);
  int _printCaps(AndorCapabilities &caps);
  int _runAcquisition();

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
  int     _iTriggerMode;
  string  _strFnPrefix;
  int     _iNumImages;
  int     _iOutputMode;
  int     _iMenu;
  bool    _bTriggerFast;
  bool    _bCropMode;

  int     _iOutImageIndex;
  at_32   _iCameraHandle;
  int     _iDetectorWidth;
  int     _iDetectorHeight;
  int     _iADChannel;

  // Class usage control: Value semantics is disabled
  AndorCameraTest(const AndorCameraTest &);
  AndorCameraTest & operator=(const AndorCameraTest &);
};

AndorCameraTest::AndorCameraTest(int iCamera, double fExposureTime, int iReadoutPort, int iSpeedIndex,
  int iGainIndex, double fTemperature, int iRoiX, int iRoiY, int iRoiW, int iRoiH, int iBinX, int iBinY,
  int iTriggerMode, char* sFnPrefix, int iNumImages, int iOutputMode, int iMenu, bool bTriggerFast, bool bCropMode) :
  _iCamera(iCamera), _fExposureTime(fExposureTime), _iReadoutPort(iReadoutPort), _iSpeedIndex(iSpeedIndex),
  _iGainIndex(iGainIndex), _fTemperature(fTemperature), _iRoiX(iRoiX), _iRoiY(iRoiY), _iRoiW(iRoiW),
  _iRoiH(iRoiH), _iBinX(iBinX), _iBinY(iBinY),  _iTriggerMode(iTriggerMode), _strFnPrefix(sFnPrefix),
  _iNumImages(iNumImages), _iOutputMode(iOutputMode), _iMenu(iMenu), _bTriggerFast(bTriggerFast), _bCropMode(bCropMode),
  _iOutImageIndex(0), _iCameraHandle(0), _iDetectorWidth(-1), _iDetectorHeight(-1), _iADChannel(0)
{
}

static const char* lsTriggerMode[] =
{ "Internal", //0
  "External", //1
  "", "", "", "",
  "External Start", //6
  "External Exposure (Bulb)", //7
  "",
  "External FVB EM", //9
  "Software Trigger", //10
  "",
  "External Charge Shifting", //12
};

static const char* lsOutputMode[] =
{
  "RAW",
  "SIF",
};

static const char* lsOutputModeExt[] =
{
  "raw",
  "sif",
};

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

  timespec timeVal0;
  clock_gettime( CLOCK_REALTIME, &timeVal0 );

  iError = Initialize(const_cast<char*>("/usr/local/etc/andor"));
  if (isAndorFuncOk(iError))
  {
    printf("Waiting for hardware to finish initializing...\n");
    sleep(2); //sleep to allow initialization to complete
    iCameraInitialized = 1;
  }
  else
  {
    printf("Initialize(): %s\n", AndorErrorCodes::name(iError));
    return 2;
  }

  timespec timeVal1;
  clock_gettime( CLOCK_REALTIME, &timeVal1 );

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
  iError = GetCameraSerialNumber(&iSerialNumber);
  if (isAndorFuncOk(iError))
    printf("Camera serial number: %d\n", iSerialNumber);
  else
    printf("GetCameraSerialNumber(): %s\n", AndorErrorCodes::name(iError));

  char sHeadModel[256];
  iError = GetHeadModel(sHeadModel);
  if (isAndorFuncOk(iError))
    printf("Camera Head Model: %s\n", sHeadModel);
  else
    printf("GetHeadModel(): %s\n", AndorErrorCodes::name(iError));

  //Get Detector dimensions
  GetDetector(&_iDetectorWidth, &_iDetectorHeight);

  float fPixelWidth = -1, fPixelHeight = -1;
  GetPixelSize(&fPixelWidth, &fPixelHeight);
  printf("Detector Width %d Height %d  Pixel Width (um) %.2f Height %.2f\n",
    _iDetectorWidth, _iDetectorHeight, fPixelWidth, fPixelHeight);

  AndorCapabilities caps;
  iError = GetCapabilities(&caps);
  if (isAndorFuncOk(iError))
    _printCaps(caps);
  else
    printf("GetCapabilities(): %s\n", AndorErrorCodes::name(iError));

  printf("Available Trigger Modes:\n");
  for (int iTriggerMode = 0; iTriggerMode < 13; ++iTriggerMode)
  {
    iError = IsTriggerModeAvailable(iTriggerMode);
    if (isAndorFuncOk(iError))
      printf("  [%d] %s\n", iTriggerMode, lsTriggerMode[iTriggerMode]);
  }

  int iNumVSSpeed = -1;
  GetNumberVSSpeeds(&iNumVSSpeed);
  printf("VSSpeed Number: %d\n", iNumVSSpeed);

  for (int iVSSpeed = 0; iVSSpeed < iNumVSSpeed; ++iVSSpeed)
  {
    float fSpeed;
    GetVSSpeed(iVSSpeed, &fSpeed);
    printf("  VSSpeed[%d] : %f us/pixel\n", iVSSpeed, fSpeed);
  }

  int   iVSRecIndex = -1;
  float fVSRecSpeed = -1;
  GetFastestRecommendedVSSpeed(&iVSRecIndex, &fVSRecSpeed);
  printf("VSSpeed Recommended Index [%d] Speed %f us/pixel\n", iVSRecIndex, fVSRecSpeed);

  iError = SetVSSpeed(iVSRecIndex);
  if (isAndorFuncOk(iError))
    printf("Set VSSpeed to %d\n", iVSRecIndex);
  else
    printf("SetVSSpeed(): %s\n", AndorErrorCodes::name(iError));

  int iNumVSAmplitude = -1;
  GetNumberVSAmplitudes(&iNumVSAmplitude);
  printf("VSAmplitude Number: %d\n", iNumVSAmplitude);

  for (int iVSAmplitude = 0; iVSAmplitude < iNumVSAmplitude; ++iVSAmplitude)
  {
    int iAmplitudeValue = -1;
    GetVSAmplitudeValue(iVSAmplitude, &iAmplitudeValue);

    char sAmplitude[32];
    sAmplitude[sizeof(sAmplitude)-1] = 0;
    GetVSAmplitudeString(iVSAmplitude, sAmplitude);
    printf("  VSAmplitude[%d]: [%d] %s\n", iVSAmplitude, iAmplitudeValue, sAmplitude);
  }

  int iNumGain = -1;
  GetNumberPreAmpGains(&iNumGain);
  printf("Preamp Gain Number: %d\n", iNumGain);

  for (int iGain = 0; iGain < iNumGain; ++iGain)
  {
    float fGain = -1;
    GetPreAmpGain(iGain, &fGain);

    char sGainText[64];
    sGainText[sizeof(sGainText)-1] = 0;
    GetPreAmpGainText(iGain, sGainText, sizeof(sGainText));
    printf("  Gain %d: %s\n", iGain, sGainText);
  }

  int iNumChannel = -1;
  GetNumberADChannels(&iNumChannel);
  printf("Channel Number: %d\n", iNumChannel);

  int iNumAmp = -1;
  GetNumberAmp(& iNumAmp);
  printf("Amp Number: %d\n", iNumAmp);

  for (int iChannel = 0; iChannel < iNumChannel; ++iChannel)
  {
    printf("  Channel[%d]\n", iChannel);

    int iDepth = -1;
    GetBitDepth(iChannel, &iDepth);
    printf("    Depth %d\n", iDepth);

    for (int iAmp = 0; iAmp < iNumAmp; ++iAmp)
    {
      printf("    Amp[%d]\n", iAmp);
      int iNumHSSpeed = -1;
      GetNumberHSSpeeds(iChannel, iAmp, &iNumHSSpeed);

      for (int iSpeed = 0; iSpeed < iNumHSSpeed; ++iSpeed)
      {
        float fSpeed = -1;
        GetHSSpeed(iChannel, iAmp, iSpeed, &fSpeed);
        printf("      Speed[%d]: %f MHz\n", iSpeed, fSpeed);

        for (int iGain = 0; iGain < iNumGain; ++iGain)
        {
          int iStatus = -1;
          IsPreAmpGainAvailable(iChannel, iAmp, iSpeed, iGain, &iStatus);
          printf("        Gain [%d]: %d\n", iGain, iStatus);
        }
      }
    }
  }

  _iADChannel = 0; // hard coded to channel 0
  if (_iReadoutPort == -1) _iReadoutPort  = 0;
  if (_iSpeedIndex  == -1) _iSpeedIndex   = 0;
  if (_iGainIndex   == -1) _iGainIndex    = 0;

  int iTempMin = -1;
  int iTempMax = -1;
  GetTemperatureRange(&iTempMin, &iTempMax);
  printf("Temperature Min %d Max %d\n", iTempMin, iTempMax);

  int iFrontEndStatus = -1;
  int iTECStatus      = -1;
  GetFrontEndStatus (&iFrontEndStatus);
  GetTECStatus      (&iTECStatus);
  printf("Overheat: FrontEnd %d TEC %d\n", iFrontEndStatus, iTECStatus);

  int iCoolerStatus = -1;
  iError = IsCoolerOn(&iCoolerStatus);
  if (!isAndorFuncOk(iError))
    printf("IsCoolerOn(): %s\n", AndorErrorCodes::name(iError));

  int iTemperature = -1;
  iError = GetTemperature(&iTemperature);
  printf("Current Temperature %d C  Status %s Cooler %d\n", iTemperature, AndorErrorCodes::name(iError), iCoolerStatus);

  float fSensorTemp   = -1;
  float fTargetTemp   = -1;
  float fAmbientTemp  = -1;
  float fCoolerVolts  = -1;
  GetTemperatureStatus(&fSensorTemp, &fTargetTemp, &fAmbientTemp, &fCoolerVolts);
  printf("Advanced Temperature: Sensor %f Target %f Ambient %f CoolerVolts %f\n", fSensorTemp, fTargetTemp, fAmbientTemp, fCoolerVolts);

  int iFanMode = 0; // 0: Full, 1: Low: 2:Off
  iError = SetFanMode(iFanMode);
  if (!isAndorFuncOk(iError))
    printf("SetFanMode(%d): %s\n", iFanMode, AndorErrorCodes::name(iError));
  else
    printf("Set Fan Mode      to %d\n", iFanMode);

  int iBaselineClamp = 0;
  iError = SetBaselineClamp(iBaselineClamp);
  if (!isAndorFuncOk(iError))
    printf("SetBaselineClamp(): %s\n", AndorErrorCodes::name(iError));
  else
    printf("Set BaselineClamp to %d\n", iBaselineClamp);

  int iHighCapacity = 0;
  iError = SetHighCapacity(iHighCapacity);
  if (!isAndorFuncOk(iError))
    printf("SetHighCapacity(): %s\n", AndorErrorCodes::name(iError));
  else
    printf("Set HighCapacity  to %d\n", iHighCapacity);

  /*
   * setup for acquisition
   */

  int iMaxBinH = -1;
  int iMaxBinV = -1;
  GetMaximumBinning(4, 0, &iMaxBinH);
  GetMaximumBinning(4, 1, &iMaxBinV);
  printf("Max binning in Image mode: H %d V %d\n", iMaxBinH, iMaxBinV);

  GetMaximumBinning(0, 0, &iMaxBinH);
  GetMaximumBinning(0, 1, &iMaxBinV);
  printf("Max binning in Full Vertical Binning mode: H %d V %d\n", iMaxBinH, iMaxBinV);

  float fTimeMaxExposure;
  GetMaximumExposure(&fTimeMaxExposure);
  printf("Max exposure time: %f s\n", fTimeMaxExposure);

  int iMinImageLen = -1;
  GetMinimumImageLength (&iMinImageLen);
  printf("MinImageLen: %d\n", iMinImageLen);

  if (_iRoiW < 0)
    _iRoiW = _iDetectorWidth - _iRoiX;
  if (_iRoiH < 0)
    _iRoiH = _iDetectorHeight - _iRoiY;

  //Initialize Shutter
  SetShutter(1,0,0,0);

  iError = SetDMAParameters(1, 0.001);
  if (!isAndorFuncOk(iError))
    printf("SetDMAParameters(): %s\n", AndorErrorCodes::name(iError));
  else
    printf("SetDMAParameters() successfully.\n");

  printf("Initialization okay\n");

  timespec timeVal2;
  clock_gettime( CLOCK_REALTIME, &timeVal2 );
  double fTimeStartup = (timeVal1.tv_nsec - timeVal0.tv_nsec) * 1.e-6 + ( timeVal1.tv_sec - timeVal0.tv_sec ) * 1.e3;
  double fTimeInfo    = (timeVal2.tv_nsec - timeVal1.tv_nsec) * 1.e-6 + ( timeVal2.tv_sec - timeVal1.tv_sec ) * 1.e3;
  printf(">> Startup time %6.1lf  Info time %6.1lf ms\n", fTimeStartup, fTimeInfo);

  return 0;
}

int AndorCameraTest::_selectCamera(int iCamera)
{
  at_32 iNumCamera;
  unsigned int iError = GetAvailableCameras(&iNumCamera);
  if (!isAndorFuncOk(iError))
    printf("GetAvailableCameras(): %s\n", AndorErrorCodes::name(iError));
  else
    printf("Found %d Andor Cameras\n", (int) iNumCamera);

  iError = GetCameraHandle(iCamera, &_iCameraHandle);
  if (!isAndorFuncOk(iError))
    printf("GetCameraHandle(): %s\n", AndorErrorCodes::name(iError));

  iError = SetCurrentCamera(_iCameraHandle);
  if (!isAndorFuncOk(iError))
    printf("SetCurrentCamera(): %s\n", AndorErrorCodes::name(iError));

  return 0;
}

int AndorCameraTest::run()
{
  if (_iMenu == 0)
    return _runAcquisition();

  bool quit = false;
  do{
    //Show menu options
    cout << endl;
    cout << "======= Current Configuration ======="  << endl;
    cout << "Output Filename Prefix  : " << _strFnPrefix    << endl;
    cout << "Number Images per Acq   : " << _iNumImages     << endl;
    cout << "Exposure Time (sec)     : " << _fExposureTime  << endl;
    cout << "Readout Port            : " << _iReadoutPort   << endl;
    cout << "Speed Index             : " << _iSpeedIndex    <<  endl;
    cout << "Gain Index              : " << _iGainIndex     << endl;
    cout << "Cooling Temperature (C) : " << _fTemperature   << endl;
    cout << "Trigger Mode            : " << lsTriggerMode[_iTriggerMode] << endl;
    cout << "Fast Trigger Mode       : " << boolalpha << _bTriggerFast << noboolalpha << endl;
    cout << "Isolated Crop Mode      : " << boolalpha << _bCropMode << noboolalpha << endl;
    cout << "Output File Mode        : " << lsOutputMode[_iOutputMode] << endl;
    cout << "ROI : x " << _iRoiX << " y " <<  _iRoiY <<
      " W " << _iRoiW << " H " << _iRoiH <<
      " binX " << _iBinX << " binY " << _iBinY << endl;
    cout << "=============== Menu ================"  << endl;
    cout << "a. Start Acquisition" << endl;
    cout << "w. Set Output Filename Prefix" << endl;
    cout << "n. Set Number of Images per Acquisition" << endl;
    cout << "e. Set Exposure Time" << endl;
    cout << "p. Set Readout Port" << endl;
    cout << "s. Set Speed Index" << endl;
    cout << "g. Set Gain Index" << endl;
    cout << "t. Set Cooling Temperature" << endl;
    cout << "i. Set Trigger Mode" << endl;
    cout << "r. Set ROI" << endl;
    cout << "b. Set Binning" << endl;
    cout << "f. Set Fast Trigger Mode" << endl;
    cout << "x. Set Isolated Crop Mode" << endl;
    cout << "o. Set Output File Mode" << endl;
    cout << "q. Quit Program" << endl;
    cout << "====================================="  << endl;
    cout << "> ";
    //Get menu choice
    int choice = getchar();

    switch(choice){
    case 'a': //Acquire
      _runAcquisition();
      break;
    case 'w':
    {
      cout << endl << "Enter new Output Filename Prefix ('x': disable output) > ";
      cin >> _strFnPrefix;
      if (_strFnPrefix == "x")
        _strFnPrefix = "";
      printf("New Output Filename Prefix: \'%s\'\n", _strFnPrefix.c_str());
      break;
    }
    case 'n':
    {
      cout << endl << "Enter new Number of Images per Acquisition > ";
      cin >> _iNumImages;
      printf("New Number of Images per Acquisition: %d\n", _iNumImages);
      break;
    }
    case 'e':
    {
      cout << endl << "Enter new Exposure Time (sec) > ";
      cin >> _fExposureTime;
      printf("New Exposure Time: %f\n", _fExposureTime);
      break;
    }
    case 'p':
    {
      cout << endl << "Enter new Readout Port > ";
      cin >> _iReadoutPort;
      printf("New Readout Port: %d\n", _iReadoutPort);
      break;
    }
    case 's':
    {
      cout << endl << "Enter new Speed Index > ";
      cin >> _iSpeedIndex;
      printf("New Speed Index: %d\n", _iSpeedIndex);
      break;
    }
    case 'g':
    {
      cout << endl << "Enter new Gain Index > ";
      cin >> _iGainIndex;
      printf("New Gain Index: %d\n", _iGainIndex);
      break;
    }
    case 't':
    {
      cout << endl << "Enter new Cooling Temperature (C) > ";
      cin >> _fTemperature;
      printf("New Cooling Temperature: %f\n", _fTemperature);
      break;
    }
    case 'i':
    {
      cout << endl << "Trigger Mode:" << endl;
      cout <<         "  0: Int, 1: Ext, 6: ExtStart, 7: Bulb," << endl;
      cout <<         "  9: ExtFvbEm, 10: Soft, 12: ExtChrgSht" << endl;
      cout << endl << "Enter new Trigger Mode > ";
      cin >> _iTriggerMode;
      printf("New Trigger Mode: %d\n", _iTriggerMode);
      break;
    }
    case 'o':
    {
      cout << endl << "Output File Mode:" << endl;
      cout <<         "  0: RAW, 1: SIF" << endl;
      cout << endl << "Enter new Output File Mode > ";
      cin >> _iOutputMode;
      printf("New Output File Mode: %d\n", _iOutputMode);
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
      _iRoiX = strtoul(pNextToken, &pNextToken, 0); ++pNextToken;
      if ( *pNextToken == 0 ) break;
      _iRoiY = strtoul(pNextToken, &pNextToken, 0); ++pNextToken;
      if ( *pNextToken == 0 ) break;
      _iRoiW = strtoul(pNextToken, &pNextToken, 0); ++pNextToken;
      if ( *pNextToken == 0 ) break;
      _iRoiH = strtoul(pNextToken, &pNextToken, 0); ++pNextToken;
      printf("New ROI: x %d y %d W %d H %d\n", _iRoiX, _iRoiY, _iRoiW, _iRoiH);
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
      _iBinX = strtoul(pNextToken, &pNextToken, 0); ++pNextToken;
      if ( *pNextToken == 0 ) break;
      _iBinY = strtoul(pNextToken, &pNextToken, 0); ++pNextToken;
      printf("New binX %d binY %d \n", _iBinX, _iBinY);
      break;
    }
    case 'f':
    {
      cout << endl << "Enter new Fast Trigger Mode [true, false]> ";
      cin >> _bTriggerFast;
      break;
    }
    case 'x':
    {
      cout << endl << "Enter new Crop Mode [true, false]> ";
      cin >> _bCropMode;
      break;
    }
    case 'q': //Exit
      quit = true;
      break;

    default:
      cout << "!Invalid Option! Key = " << choice << endl;
    }
    getchar();

  }
  while(!quit);

  return 0;
}

int AndorCameraTest::_runAcquisition()
{
  int iError;

  timespec timeVala0;
  clock_gettime( CLOCK_REALTIME, &timeVala0 );

  int iTempMin = -1;
  int iTempMax = -1;
  GetTemperatureRange(&iTempMin, &iTempMax);
  if ( _fTemperature < iTempMin || _fTemperature > iTempMax )
    printf("Cooling temperature %f out of range (min %d max %d)\n", _fTemperature, iTempMin, iTempMax);
  else
  {
    iError = SetTemperature((int)_fTemperature);
    if (!isAndorFuncOk(iError))
    {
      printf("SetTemperature(): %s\n", AndorErrorCodes::name(iError));
      return 1;
    }
    iError = CoolerON();
    if (!isAndorFuncOk(iError))
    {
      printf("CoolerON(): %s\n", AndorErrorCodes::name(iError));
      return 2;
    }
    printf("Set Temperature to %f C\n", _fTemperature);
  }

  int iCoolerStatus = -1;
  iError = IsCoolerOn(&iCoolerStatus);
  if (!isAndorFuncOk(iError))
    printf("IsCoolerOn(): %s\n", AndorErrorCodes::name(iError));

  int iTemperature;
  iError = GetTemperature(&iTemperature);
  printf("Current Temperature %d C  Status %s Cooler %d\n", iTemperature, AndorErrorCodes::name(iError), iCoolerStatus);

  iError = SetADChannel(_iADChannel);
  if (!isAndorFuncOk(iError))
  {
    printf("SetADChannel(): %s\n", AndorErrorCodes::name(iError));
    return 1;
  }
  int iDepth = -1;
  GetBitDepth(_iADChannel, &iDepth);
  printf("Set Channel to %d: depth %d\n", _iADChannel, iDepth);

  int iNumAmp = -1;
  GetNumberAmp(& iNumAmp);
  if (_iReadoutPort < 0 || _iReadoutPort >= iNumAmp)
  {
    printf("Readout Port %d out of range (max index %d)\n", _iReadoutPort, iNumAmp);
    return 2;
  }
  iError = SetOutputAmplifier(_iReadoutPort);
  if (!isAndorFuncOk(iError))
  {
    printf("SetOutputAmplifier(): %s\n", AndorErrorCodes::name(iError));
    return 3;
  }
  printf("Set Readout Port to %d\n", _iReadoutPort);

  int iNumHSSpeed = -1;
  GetNumberHSSpeeds(_iADChannel, _iReadoutPort, &iNumHSSpeed);
  if (_iSpeedIndex < 0 || _iSpeedIndex >= iNumHSSpeed)
  {
    printf("Speed Index %d out of range (max index %d)\n", _iSpeedIndex, iNumHSSpeed);
    return 4;
  }

  iError = SetHSSpeed(_iReadoutPort, _iSpeedIndex);
  if (!isAndorFuncOk(iError))
  {
    printf("SetHSSpeed(): %s\n", AndorErrorCodes::name(iError));
    return 5;
  }
  float fSpeed = -1;
  GetHSSpeed(_iADChannel, _iReadoutPort, _iSpeedIndex, &fSpeed);
  printf("Set Speed Index to %d: %f MHz\n", _iSpeedIndex, fSpeed);

  int iNumGain = -1;
  GetNumberPreAmpGains(&iNumGain);
  if (_iGainIndex < 0 || _iGainIndex >= iNumGain)
  {
    printf("Gain Index %d out of range (max index %d)\n", _iGainIndex, iNumGain);
    return 6;
  }

  int iStatus = -1;
  IsPreAmpGainAvailable(_iADChannel, _iReadoutPort, _iSpeedIndex, _iGainIndex, &iStatus);
  if (iStatus != 1)
  {
    printf("Gain Index %d not supported for channel %d port %d speed %d\n", _iGainIndex,
      _iADChannel, _iReadoutPort, _iSpeedIndex);
    return 7;
  }

  iError = SetPreAmpGain(_iGainIndex);
  if (!isAndorFuncOk(iError))
  {
    printf("SetGain(): %s\n", AndorErrorCodes::name(iError));
    return 8;
  }

  float fGain = -1;
  GetPreAmpGain(_iGainIndex, &fGain);

  char sGainText[64];
  sGainText[sizeof(sGainText)-1] = 0;
  GetPreAmpGainText(_iGainIndex, sGainText, sizeof(sGainText));
  printf("Set Gain Index to %d: %s\n", _iGainIndex, sGainText);

  static HistReport   histTimeFrame   (0,10000,100);
  static HistReport   histTimeOverhead(0,5000,100);
  histTimeFrame.reset();
  histTimeOverhead.reset();

  int iImageWidth   = _iRoiW / _iBinX;
  int iImageHeight  = _iRoiH / _iBinY;
  printf("Output image W %d H %d\n", iImageWidth, iImageHeight);

  int iReadMode = 4;

  static int iAcqCall = 0;
  ++iAcqCall;
  if ( iImageWidth == _iDetectorWidth &&
       iImageHeight == 1)
  {
    if (_iRoiY == 0 && _iRoiH == _iDetectorHeight)
      iReadMode = 0;
    else
      iReadMode = 3;
  }

  /*
   * Read Mode:
   *   0: Full Vertical Binning 1: MultiTrack 2: Random Track
   *   3: Single Track 4: Image
   */
  SetReadMode(iReadMode);
  printf("Read mode: %d\n", iReadMode);

  if (iReadMode == 4)
  {
    if (_bCropMode)
    {
      //Setup Isolated Crop Mode dimensions
      printf("Setting Isolated Crop Dimensions W %d H %d binX %d binY %d\n", _iRoiW, _iRoiH, _iBinX, _iBinY);
      iError = SetIsolatedCropMode(1, _iRoiH, _iRoiW, _iBinY, _iBinX);
      if (!isAndorFuncOk(iError))
        printf("SetIsolatedCropMode(): %s\n", AndorErrorCodes::name(iError));
    }
    else {
      //Setup Image dimensions
      printf("Setting Image ROI x %d y %d W %d H %d binX %d binY %d\n", _iRoiX, _iRoiY, _iRoiW, _iRoiH, _iBinX, _iBinY);
      iError = SetIsolatedCropMode(0, _iRoiH, _iRoiW, _iBinY, _iBinX);
      if (!isAndorFuncOk(iError))
        printf("SetIsolatedCropMode(): %s\n", AndorErrorCodes::name(iError));
      iError = SetImage(_iBinX, _iBinY, _iRoiX + 1, _iRoiX + _iRoiW, _iRoiY + 1, _iRoiY + _iRoiH);
      if (!isAndorFuncOk(iError))
        printf("SetImage(): %s\n", AndorErrorCodes::name(iError));
    }
  }
  else if (iReadMode == 3) // single track
  {
    //Setup Image dimensions
    printf("Setting Single Track center %d height %d\n", _iRoiY + _iRoiH/2, _iRoiH);
    iError = SetSingleTrack( 1 + _iRoiY + _iRoiH/2, _iRoiH);
    if (!isAndorFuncOk(iError))
      printf("SetSingleTrack(): %s\n", AndorErrorCodes::name(iError));
  }

  bool bExtTrigger = (_iTriggerMode != 0 && _iTriggerMode != 10);
  if (bExtTrigger) // not Internal or Soft trigger mode
    iError = SetAcquisitionMode(5); //Set Acquisition mode to --Run Till Abort--
  else
    iError = SetAcquisitionMode(1); //Set Acquisition mode to --Single scan--

  if (!isAndorFuncOk(iError))
    printf("SetAcquisitionMode(): %s\n", AndorErrorCodes::name(iError));

  iError = SetTriggerMode(_iTriggerMode);
  if (!isAndorFuncOk(iError))
    printf("SetTriggerMode(): %s\n", AndorErrorCodes::name(iError));
  if (bExtTrigger && _bTriggerFast)
  {
    iError = SetFastExtTrigger(1);
    if (!isAndorFuncOk(iError))
      printf("SetFastExtTrigger(1): %s\n", AndorErrorCodes::name(iError));
  }

  //Set initial exposure time
  iError = SetExposureTime((float)_fExposureTime);
  if (!isAndorFuncOk(iError))
    printf("SetExposureTime(): %s\n", AndorErrorCodes::name(iError));

  if (bExtTrigger) // not Internal or Soft trigger mode
  {
    iError = SetKineticCycleTime(0);
    if (!isAndorFuncOk(iError))
      printf("SetKineticCycleTime(): %s\n", AndorErrorCodes::name(iError));

    at_32 sizeBuffer;
    iError = GetSizeOfCircularBuffer(&sizeBuffer);
    if (!isAndorFuncOk(iError))
      printf("GetSizeOfCircularBuffer(): %s\n", AndorErrorCodes::name(iError));
    else
      printf("Size of Circular Buffer: %d\n", (int) sizeBuffer);

    //** Set vertical shift speed to the fastest
    int iVSRecIndex = 0;
    iError = SetVSSpeed(iVSRecIndex);
    if (isAndorFuncOk(iError))
      printf("Set VSSpeed to %d\n", iVSRecIndex);
    else
      printf("SetVSSpeed(): %s\n", AndorErrorCodes::name(iError));

    //** Keep clean enable only available for FVB trigger mode
    int iKeepCleanMode = 0; // Off
    iError = EnableKeepCleans(iKeepCleanMode);
    if (!isAndorFuncOk(iError))
      printf("EnableKeepCleans(): %s\n", AndorErrorCodes::name(iError));

    float fTimeKeepClean = -1;
    GetKeepCleanTime(&fTimeKeepClean);
    printf("Keep clean time: %f s\n", fTimeKeepClean);
  }

  float fTimeExposure   = -1;
  float fTimeAccumulate = -1;
  float fTimeKinetic    = -1;
  GetAcquisitionTimings(&fTimeExposure, &fTimeAccumulate, &fTimeKinetic);
  printf("Exposure time: %f s  Accumulate time: %f s  Kinetic time: %f s\n", fTimeExposure, fTimeAccumulate, fTimeKinetic);

  float fTimeReadout = -1;
  GetReadOutTime(&fTimeReadout);
  printf("Readout time: %f s\n", fTimeReadout);

  //uint16_t* liImageData = new uint16_t[iImageWidth*iImageHeight];
  const int iTestBufferSizeIn16 = _iRoiW*_iRoiH*2;
  const unsigned char uTestByte = 0x77;
  uint16_t* liImageData = new uint16_t[iTestBufferSizeIn16];
  memset(liImageData, uTestByte, iTestBufferSizeIn16 * sizeof(uint16_t));

  timespec timeVala1;
  clock_gettime( CLOCK_REALTIME, &timeVala1 );

  iError = PrepareAcquisition();
  if (!isAndorFuncOk(iError))
  {
    printf("PrepareAcquisition(): %s\n", AndorErrorCodes::name(iError));
    return 9;
  }


  if (bExtTrigger)
  {
    iError = StartAcquisition();
    if (!isAndorFuncOk(iError))
      printf("StartAcquisition(): %s\n", AndorErrorCodes::name(iError));
  }

  timespec timeVala2;
  clock_gettime( CLOCK_REALTIME, &timeVala2 );

  double fTimeSetupAcq  = (timeVala1.tv_nsec - timeVala0.tv_nsec) * 1.e-6 + ( timeVala1.tv_sec - timeVala0.tv_sec ) * 1.e3;
  double fTimePreAcq    = (timeVala2.tv_nsec - timeVala1.tv_nsec) * 1.e-6 + ( timeVala2.tv_sec - timeVala1.tv_sec ) * 1.e3;
  printf(">> Acquisition Setup Time %6.1lf  Preparation Time %6.1lf ms\n", fTimeSetupAcq, fTimePreAcq);

  bool    bPrint    = true;
  double  fAccTime  = 0;
  at_32   numAcq    = 0;
  at_32   numAcqNew = 0;
  for (int iImage = 0; iImage < _iNumImages; ++iImage)
  {
    if (bPrint)
      printf("Image [%3d/%3d]", 1+iImage, _iNumImages);
    //fflush(NULL);

    timespec timeVal0;
    clock_gettime( CLOCK_REALTIME, &timeVal0 );

    if (!bExtTrigger)
    {
      iError = StartAcquisition();
      if (!isAndorFuncOk(iError))
        printf("StartAcquisition(): %s\n", AndorErrorCodes::name(iError));
    }

    timespec timeVal1;
    clock_gettime( CLOCK_REALTIME, &timeVal1 );

    ////Loop until acquisition finished
    //int status;
    //GetStatus(&status);
    //while(status==DRV_ACQUIRING) GetStatus(&status);
    //if (status != DRV_IDLE)
    //  printf("GetStatus() return status %s\n", AndorErrorCodes::name(status));

    bool      bWaitError      = false;
    const int iMaxReadoutTime = 12000; // in ms
    if (!bExtTrigger)
    {
      iError = WaitForAcquisitionTimeOut(iMaxReadoutTime );
      if (!isAndorFuncOk(iError))
      {
        printf("WaitForAcquisitionTimeOut(): %s\n", AndorErrorCodes::name(iError));
        break;
      }
    }
    else
    {
      while (numAcqNew == numAcq)
      {
        iError = GetTotalNumberImagesAcquired(&numAcqNew);
        if (!isAndorFuncOk(iError))
        {
          printf("GetTotalNumberImagesAcquired(): [%d] %s\n", iError, AndorErrorCodes::name(iError));
          bWaitError = true;
          break;
        }

        if (numAcqNew != numAcq)
        {
          if (bPrint)
            printf("Num Image Acquired: %d / %d\n", (int) numAcqNew, (int) numAcq);
        }
        else
        {
          iError = WaitForAcquisitionTimeOut(iMaxReadoutTime);
          if (!isAndorFuncOk(iError))
          {
            printf("WaitForAcquisitionTimeOut(): %s\n", AndorErrorCodes::name(iError));
            bWaitError = true;
            break;
          }
        }
      }
      ++numAcq;
    }

    timespec timeVal2;
    clock_gettime( CLOCK_REALTIME, &timeVal2 );

    if (bWaitError)
      break;

    //iError = GetAcquiredData16(liImageData, iImageWidth*iImageHeight);
    const int iAcqBufferSize = iImageWidth*iImageHeight;
    //printf( "GetAcquiredData16(...,%d) size(uint16) = %d\n", iAcqBufferSize, iImageWidth*iImageHeight);

    //if (!bExtTrigger)
    //  iError = GetAcquiredData16(liImageData, iAcqBufferSize);
    //else
    iError = GetOldestImage16(liImageData, iAcqBufferSize);
    if (!isAndorFuncOk(iError))
      printf("GetOldestImage16(): %s\n", AndorErrorCodes::name(iError));
    else
    {
      if (!bExtTrigger)
      {
        const uint16_t*     pPixel     = liImageData;
        const uint16_t*     pEnd       = pPixel + iImageWidth*iImageHeight;
        const uint64_t      uNumPixels = (uint64_t) (iImageWidth*iImageHeight);

        uint64_t            uSum    = 0;
        uint64_t            uSumSq  = 0;
        for ( ; pPixel < pEnd; pPixel++ )
        {
          uSum   += *pPixel;
          uSumSq += ((uint32_t)*pPixel) * ((uint32_t)*pPixel);
        }

        printf( " Avg %.2lf Std %.2lf",
          (double) uSum / (double) uNumPixels,
          sqrt( (uNumPixels * uSumSq - uSum * uSum) / (double)(uNumPixels*uNumPixels)) );

        char* pByte;
        for ( pByte = (char*)( liImageData + iTestBufferSizeIn16 ) - 1;
              pByte >= (char*)liImageData && *pByte == uTestByte; --pByte )
          ;
        printf( " Image size (bytes): %ld\n", (long) (pByte - (char*) liImageData) + 1 );
      }

      if ( !_strFnPrefix.empty() )
      {
        ++_iOutImageIndex;

        char sFnOut[32];
        sprintf(sFnOut, "%s_%03d.%s", _strFnPrefix.c_str(), _iOutImageIndex, lsOutputModeExt[_iOutputMode]);

        printf("Writing to image file %s...", sFnOut);
        fflush(NULL);
        printf("done.\n");

        if (_iOutputMode == 0) {
          int fdImage = ::open(sFnOut, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH );
          ::write(fdImage, liImageData, iImageWidth*iImageHeight*sizeof(liImageData[0]));
          ::close(fdImage);
        } else if (_iOutputMode == 1) {
          SaveAsSif(sFnOut);
        } else {
          cout << "Unsupported output file type: " << _iOutputMode << endl;
        }
      }
    }
    //fstream fout("image.txt", ios::out);
    //for(int i=0;i<_iDetectorWidth*_iDetectorHeight;i++) fout << imageData[i] << endl;
    //SaveAsBmp("./image.bmp", "./GREY.PAL", 0, 0);

    timespec timeVal3;
    clock_gettime( CLOCK_REALTIME, &timeVal3 );

    //if (bExtTrigger) // not Internal or Soft trigger mode
    //{
    //  at_32 numFirst, numLast;
    //  iError = GetNumberNewImages(&numFirst, &numLast);
    //  if (!isAndorFuncOk(iError))
    //    printf("GetNumberNewImages(): %s\n", AndorErrorCodes::name(iError));
    //  else
    //    printf("New Image: first %d last %d\n", (int)numFirst, (int)numLast);
    //}

    double fTimeInit  = (timeVal1.tv_nsec - timeVal0.tv_nsec) * 1.e-6 + ( timeVal1.tv_sec - timeVal0.tv_sec ) * 1.e3;
    double fTimeAcq   = (timeVal2.tv_nsec - timeVal1.tv_nsec) * 1.e-6 + ( timeVal2.tv_sec - timeVal1.tv_sec ) * 1.e3;
    double fTimePost  = (timeVal3.tv_nsec - timeVal2.tv_nsec) * 1.e-6 + ( timeVal3.tv_sec - timeVal2.tv_sec ) * 1.e3;
    double fTimeFrame = fTimeInit + fTimeAcq;
    double fTimeOverhead = fTimeFrame - (_fExposureTime + fTimeReadout) * 1000;
    if (bPrint)
      printf(" Time Init %6.1lf  Acq %6.1lf  Post %6.1f Frame %6.1lf  Overhead %6.1lf ms\n", fTimeInit, fTimeAcq, fTimePost, fTimeFrame, fTimeOverhead);

    histTimeFrame.addValue(fTimeFrame);
    histTimeOverhead.addValue(fTimeOverhead);

    if (bExtTrigger) {
      fAccTime = (bPrint ? fTimeFrame : fAccTime+fTimeFrame);
      bPrint = (fAccTime > 970);
      //printf("  accTime: %f\n", fAccTime);
    }

  }

  if (bExtTrigger) // not Internal or Soft trigger mode
  {
    iError = AbortAcquisition();
    if (!isAndorFuncOk(iError))
      printf("AbortAcquisition(): %s\n", AndorErrorCodes::name(iError));
  }

  delete[] liImageData;

  printf("\n");
  histTimeFrame.report("Frame time");
  printf("\n");
  histTimeOverhead.report("Overhead time");

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
  printf("    AC_READMODE_FULLIMAGE       : %d\n", (caps.ulReadModes & AC_READMODE_FULLIMAGE)? 1:0 );
  printf("    AC_READMODE_SUBIMAGE        : %d\n", (caps.ulReadModes & AC_READMODE_SUBIMAGE)? 1:0 );
  printf("    AC_READMODE_SINGLETRACK     : %d\n", (caps.ulReadModes & AC_READMODE_SINGLETRACK)? 1:0 );
  printf("    AC_READMODE_FVB             : %d\n", (caps.ulReadModes & AC_READMODE_FVB)? 1:0 );
  printf("    AC_READMODE_MULTITRACK      : %d\n", (caps.ulReadModes & AC_READMODE_MULTITRACK)? 1:0 );
  printf("    AC_READMODE_RANDOMTRACK     : %d\n", (caps.ulReadModes & AC_READMODE_RANDOMTRACK)? 1:0 );
  printf("    AC_READMODE_MULTITRACKSCAN  : %d\n", (caps.ulReadModes & AC_READMODE_MULTITRACKSCAN)? 1:0 );

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
  printf("    AC_PIXELMODE_16BIT  : %d\n", (caps.ulPixelMode & AC_PIXELMODE_16BIT)? 1:0 );
  printf("  SetFunctions      : 0x%x\n", (int) caps.ulSetFunctions);
  printf("    AC_SETFUNCTION_VREADOUT           : %d\n", (caps.ulSetFunctions & AC_SETFUNCTION_VREADOUT)? 1:0 );
  printf("    AC_SETFUNCTION_HREADOUT           : %d\n", (caps.ulSetFunctions & AC_SETFUNCTION_HREADOUT)? 1:0 );
  printf("    AC_SETFUNCTION_TEMPERATURE        : %d\n", (caps.ulSetFunctions & AC_SETFUNCTION_TEMPERATURE)? 1:0 );
  printf("    AC_SETFUNCTION_MCPGAIN            : %d\n", (caps.ulSetFunctions & AC_SETFUNCTION_MCPGAIN)? 1:0 );
  printf("    AC_SETFUNCTION_EMCCDGAIN          : %d\n", (caps.ulSetFunctions & AC_SETFUNCTION_EMCCDGAIN)? 1:0 );
  printf("    AC_SETFUNCTION_BASELINECLAMP      : %d\n", (caps.ulSetFunctions & AC_SETFUNCTION_BASELINECLAMP)? 1:0 );
  printf("    AC_SETFUNCTION_VSAMPLITUDE        : %d\n", (caps.ulSetFunctions & AC_SETFUNCTION_VSAMPLITUDE)? 1:0 );
  printf("    AC_SETFUNCTION_HIGHCAPACITY       : %d\n", (caps.ulSetFunctions & AC_SETFUNCTION_HIGHCAPACITY)? 1:0 );
  printf("    AC_SETFUNCTION_BASELINEOFFSET     : %d\n", (caps.ulSetFunctions & AC_SETFUNCTION_BASELINEOFFSET)? 1:0 );
  printf("    AC_SETFUNCTION_PREAMPGAIN         : %d\n", (caps.ulSetFunctions & AC_SETFUNCTION_PREAMPGAIN)? 1:0 );
  printf("    AC_SETFUNCTION_CROPMODE           : %d\n", (caps.ulSetFunctions & AC_SETFUNCTION_CROPMODE)? 1:0 );
  printf("    AC_SETFUNCTION_DMAPARAMETERS      : %d\n", (caps.ulSetFunctions & AC_SETFUNCTION_DMAPARAMETERS)? 1:0 );
  printf("    AC_SETFUNCTION_HORIZONTALBIN      : %d\n", (caps.ulSetFunctions & AC_SETFUNCTION_HORIZONTALBIN)? 1:0 );
  printf("    AC_SETFUNCTION_MULTITRACKHRANGE   : %d\n", (caps.ulSetFunctions & AC_SETFUNCTION_MULTITRACKHRANGE)? 1:0 );
  printf("    AC_SETFUNCTION_RANDOMTRACKNOGAPS  : %d\n", (caps.ulSetFunctions & AC_SETFUNCTION_RANDOMTRACKNOGAPS)? 1:0 );
  printf("    AC_SETFUNCTION_EMADVANCED         : %d\n", (caps.ulSetFunctions & AC_SETFUNCTION_EMADVANCED)? 1:0 );
  printf("    AC_SETFUNCTION_GATEMODE           : %d\n", (caps.ulSetFunctions & AC_SETFUNCTION_GATEMODE)? 1:0 );
  printf("    AC_SETFUNCTION_DDGTIMES           : %d\n", (caps.ulSetFunctions & AC_SETFUNCTION_DDGTIMES)? 1:0 );
  printf("    AC_SETFUNCTION_IOC                : %d\n", (caps.ulSetFunctions & AC_SETFUNCTION_IOC)? 1:0 );
  printf("    AC_SETFUNCTION_INTELLIGATE        : %d\n", (caps.ulSetFunctions & AC_SETFUNCTION_INTELLIGATE)? 1:0 );
  printf("    AC_SETFUNCTION_INSERTION_DELAY    : %d\n", (caps.ulSetFunctions & AC_SETFUNCTION_INSERTION_DELAY)? 1:0 );
  printf("    AC_SETFUNCTION_GATESTEP           : %d\n", (caps.ulSetFunctions & AC_SETFUNCTION_GATESTEP)? 1:0 );
  printf("    AC_SETFUNCTION_TRIGGERTERMINATION : %d\n", (caps.ulSetFunctions & AC_SETFUNCTION_TRIGGERTERMINATION)? 1:0 );
  printf("    AC_SETFUNCTION_EXTENDEDNIR        : %d\n", (caps.ulSetFunctions & AC_SETFUNCTION_EXTENDEDNIR)? 1:0 );
  printf("    AC_SETFUNCTION_SPOOLTHREADCOUNT   : %d\n", (caps.ulSetFunctions & AC_SETFUNCTION_SPOOLTHREADCOUNT)? 1:0 );

  printf("  GetFunctions      : 0x%x\n", (int) caps.ulGetFunctions);
  printf("    AC_GETFUNCTION_TEMPERATURE        : %d\n", (caps.ulGetFunctions & AC_GETFUNCTION_TEMPERATURE)? 1:0 );
  printf("    AC_GETFUNCTION_TARGETTEMPERATURE  : %d\n", (caps.ulGetFunctions & AC_GETFUNCTION_TARGETTEMPERATURE)? 1:0 );
  printf("    AC_GETFUNCTION_TEMPERATURERANGE   : %d\n", (caps.ulGetFunctions & AC_GETFUNCTION_TEMPERATURERANGE)? 1:0 );
  printf("    AC_GETFUNCTION_DETECTORSIZE       : %d\n", (caps.ulGetFunctions & AC_GETFUNCTION_DETECTORSIZE)? 1:0 );
  printf("    AC_GETFUNCTION_MCPGAIN            : %d\n", (caps.ulGetFunctions & AC_GETFUNCTION_MCPGAIN)? 1:0 );
  printf("    AC_GETFUNCTION_EMCCDGAIN          : %d\n", (caps.ulGetFunctions & AC_GETFUNCTION_EMCCDGAIN)? 1:0 );
  printf("    AC_GETFUNCTION_HVFLAG             : %d\n", (caps.ulGetFunctions & AC_GETFUNCTION_HVFLAG)? 1:0 );
  printf("    AC_GETFUNCTION_GATEMODE           : %d\n", (caps.ulGetFunctions & AC_GETFUNCTION_GATEMODE)? 1:0 );
  printf("    AC_GETFUNCTION_DDGTIMES           : %d\n", (caps.ulGetFunctions & AC_GETFUNCTION_DDGTIMES)? 1:0 );
  printf("    AC_GETFUNCTION_IOC                : %d\n", (caps.ulGetFunctions & AC_GETFUNCTION_IOC)? 1:0 );
  printf("    AC_GETFUNCTION_INTELLIGATE        : %d\n", (caps.ulGetFunctions & AC_GETFUNCTION_INTELLIGATE)? 1:0 );
  printf("    AC_GETFUNCTION_INSERTION_DELAY    : %d\n", (caps.ulGetFunctions & AC_GETFUNCTION_INSERTION_DELAY)? 1:0 );
  printf("    AC_GETFUNCTION_GATESTEP           : %d\n", (caps.ulGetFunctions & AC_GETFUNCTION_GATESTEP)? 1:0 );
  printf("    AC_GETFUNCTION_PHOSPHORSTATUS     : %d\n", (caps.ulGetFunctions & AC_GETFUNCTION_PHOSPHORSTATUS)? 1:0 );
  printf("    AC_GETFUNCTION_MCPGAINTABLE       : %d\n", (caps.ulGetFunctions & AC_GETFUNCTION_MCPGAINTABLE)? 1:0 );

  printf("  Features          : 0x%x\n", (int) caps.ulFeatures);
  printf("    AC_FEATURES_POLLING                         : %d\n", (caps.ulFeatures & AC_FEATURES_POLLING)? 1:0 );
  printf("    AC_FEATURES_EVENTS                          : %d\n", (caps.ulFeatures & AC_FEATURES_EVENTS)? 1:0 );
  printf("    AC_FEATURES_SPOOLING                        : %d\n", (caps.ulFeatures & AC_FEATURES_SPOOLING)? 1:0 );
  printf("    AC_FEATURES_SHUTTER                         : %d\n", (caps.ulFeatures & AC_FEATURES_SHUTTER)? 1:0 );
  printf("    AC_FEATURES_SHUTTEREX                       : %d\n", (caps.ulFeatures & AC_FEATURES_SHUTTEREX)? 1:0 );
  printf("    AC_FEATURES_EXTERNAL_I2C                    : %d\n", (caps.ulFeatures & AC_FEATURES_EXTERNAL_I2C)? 1:0 );
  printf("    AC_FEATURES_SATURATIONEVENT                 : %d\n", (caps.ulFeatures & AC_FEATURES_SATURATIONEVENT)? 1:0 );
  printf("    AC_FEATURES_FANCONTROL                      : %d\n", (caps.ulFeatures & AC_FEATURES_FANCONTROL)? 1:0 );
  printf("    AC_FEATURES_MIDFANCONTROL                   : %d\n", (caps.ulFeatures & AC_FEATURES_MIDFANCONTROL)? 1:0 );
  printf("    AC_FEATURES_TEMPERATUREDURINGACQUISITION    : %d\n", (caps.ulFeatures & AC_FEATURES_TEMPERATUREDURINGACQUISITION)? 1:0 );
  printf("    AC_FEATURES_KEEPCLEANCONTROL                : %d\n", (caps.ulFeatures & AC_FEATURES_KEEPCLEANCONTROL)? 1:0 );
  printf("    AC_FEATURES_DDGLITE                         : %d\n", (caps.ulFeatures & AC_FEATURES_DDGLITE)? 1:0 );
  printf("    AC_FEATURES_FTEXTERNALEXPOSURE              : %d\n", (caps.ulFeatures & AC_FEATURES_FTEXTERNALEXPOSURE)? 1:0 );
  printf("    AC_FEATURES_KINETICEXTERNALEXPOSURE         : %d\n", (caps.ulFeatures & AC_FEATURES_KINETICEXTERNALEXPOSURE)? 1:0 );
  printf("    AC_FEATURES_DACCONTROL                      : %d\n", (caps.ulFeatures & AC_FEATURES_DACCONTROL)? 1:0 );
  printf("    AC_FEATURES_METADATA                        : %d\n", (caps.ulFeatures & AC_FEATURES_METADATA)? 1:0 );
  printf("    AC_FEATURES_IOCONTROL                       : %d\n", (caps.ulFeatures & AC_FEATURES_IOCONTROL)? 1:0 );
  printf("    AC_FEATURES_PHOTONCOUNTING                  : %d\n", (caps.ulFeatures & AC_FEATURES_PHOTONCOUNTING)? 1:0 );
  printf("    AC_FEATURES_COUNTCONVERT                    : %d\n", (caps.ulFeatures & AC_FEATURES_COUNTCONVERT)? 1:0 );
  printf("    AC_FEATURES_DUALMODE                        : %d\n", (caps.ulFeatures & AC_FEATURES_DUALMODE)? 1:0 );
  printf("    AC_FEATURES_OPTACQUIRE                      : %d\n", (caps.ulFeatures & AC_FEATURES_OPTACQUIRE)? 1:0 );
  printf("    AC_FEATURES_REALTIMESPURIOUSNOISEFILTER     : %d\n", (caps.ulFeatures & AC_FEATURES_REALTIMESPURIOUSNOISEFILTER)? 1:0 );
  printf("    AC_FEATURES_POSTPROCESSSPURIOUSNOISEFILTER  : %d\n", (caps.ulFeatures & AC_FEATURES_POSTPROCESSSPURIOUSNOISEFILTER)? 1:0 );
  printf("    AC_FEATURES_DUALPREAMPGAIN                  : %d\n", (caps.ulFeatures & AC_FEATURES_DUALPREAMPGAIN)? 1:0 );
  printf("    AC_FEATURES_DEFECT_CORRECTION               : %d\n", (caps.ulFeatures & AC_FEATURES_DEFECT_CORRECTION)? 1:0 );
  printf("    AC_FEATURES_STARTOFEXPOSURE_EVENT           : %d\n", (caps.ulFeatures & AC_FEATURES_STARTOFEXPOSURE_EVENT)? 1:0 );
  printf("    AC_FEATURES_ENDOFEXPOSURE_EVENT             : %d\n", (caps.ulFeatures & AC_FEATURES_ENDOFEXPOSURE_EVENT)? 1:0 );
  printf("    AC_FEATURES_CAMERALINK                      : %d\n", (caps.ulFeatures & AC_FEATURES_CAMERALINK)? 1:0 );

  printf("  PCICard           : 0x%x\n", (int) caps.ulPCICard);
  printf("  EMGainCapability  : 0x%x\n", (int) caps.ulEMGainCapability);
  printf("  FTReadModes       : 0x%x\n", (int) caps.ulFTReadModes);
  printf("    AC_READMODE_FULLIMAGE       : %d\n", (caps.ulFTReadModes & AC_READMODE_FULLIMAGE)? 1:0 );
  printf("    AC_READMODE_SUBIMAGE        : %d\n", (caps.ulFTReadModes & AC_READMODE_SUBIMAGE)? 1:0 );
  printf("    AC_READMODE_SINGLETRACK     : %d\n", (caps.ulFTReadModes & AC_READMODE_SINGLETRACK)? 1:0 );
  printf("    AC_READMODE_FVB             : %d\n", (caps.ulFTReadModes & AC_READMODE_FVB)? 1:0 );
  printf("    AC_READMODE_MULTITRACK      : %d\n", (caps.ulFTReadModes & AC_READMODE_MULTITRACK)? 1:0 );
  printf("    AC_READMODE_RANDOMTRACK     : %d\n", (caps.ulFTReadModes & AC_READMODE_RANDOMTRACK)? 1:0 );
  printf("    AC_READMODE_MULTITRACKSCAN  : %d\n", (caps.ulFTReadModes & AC_READMODE_MULTITRACKSCAN)? 1:0 );
  return 0;
}

int closeCamera()
{
  int iError;

  iError = AbortAcquisition();
  if (!isAndorFuncOk(iError))
    printf("AbortAcquisition(): %s\n", AndorErrorCodes::name(iError));

  while (true)
  {
    float fTemperature = -999;
    iError = GetTemperatureF(&fTemperature);
    printf("Temperature %f C  Status %s\n", fTemperature, AndorErrorCodes::name(iError));

    if(fTemperature==-999 || fTemperature>5)
      break;

    cout << "Wait until temperature rises above 5C, before exiting" << endl;
  }
  iError = CoolerOFF();
  if (!isAndorFuncOk(iError))
    printf("CoolerOFF(): %s\n", AndorErrorCodes::name(iError));

  if (iCameraInitialized != 0)
  {
    ShutDown();
    iCameraInitialized = 0;
  }
  return 0;
}


static void showUsage()
{
    printf(
      "Usage:  andorStandAlone  [-v|--version] [-h|--help] [-c|--camera <camera number>]\n"
      "    [-w|--write <filename prefix>] [-n|--number <number of images>] [-e|--exposure <exposure time (sec)>]\n"
      "    [-p|--port <readout port>] [-s|--speed <speed index>] [-g|--gain <gain index>]\n"
      "    [-t|--temp <temperature>] [-r|--roi <x,y,w,h>] [-b|--bin <xbin,ybin>] [-i <trigger mode>] [-f|--fast]\n"
      "    [-x|--crop] [-m|--menu]\n"
      "  Options:\n"
      "    -v|--version                      Show file version\n"
      "    -h|--help                         Show usage\n"
      "    -c|--camera    <camera number>    Select camera\n"
      "    -w|--write     <filename prefix>  Output filename prefix\n"
      "    -n|--number    <number of images> Number of images to be captured (Default: 0)\n"
      "    -e|--exposure  <exposure time>    Exposure time (sec) (Default: 0.0001 sec)\n"
      "    -p|--port      <readout port>     Readout port\n"
      "    -s|--speed     <speed inedx>      Speed table index\n"
      "    -g|--gain      <gain index>       Gain Index\n"
      "    -t|--temp      <temperature>      Temperature (in Celcius) (Default: 25 C)\n"
      "    -r|--roi       <x,y,w,h>          Region of Interest\n"
      "    -b|--bin       <xbin,ybin>        Binning of X/Y\n"
      "    -i|--trigger   <trigger mode>     0: Int, 1: Ext, 6: ExtStart, 7: Bulb,\n"
      "                                      9: ExtFvbEm, 10: Soft, 12: ExtChrgSht\n"
      "    -f|--fast                         Enable fast trigger mode - don't wait for clear cycle\n"
      "    -x|--crop                         Enable isolated crop mode\n"
      "    -o|--output    <output format>    0: Raw, 1: SIF\n"
      "    -m|--menu                         Show interactive menu\n"
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
  const char*         strOptions  = ":vhc:w:n:e:p:s:g:t:r:b:i:fxo:m";
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
     {"trigger",  1, 0, 'i'},
     {"fast",     0, 0, 'f'},
     {"crop",     0, 0, 'x'},
     {"output",   1, 0, 'o'},
     {"menu",     0, 0, 'm'},
     {0,          0, 0,  0 }
  };

  int     iCamera        = 0;
  char    sFnPrefix[32] = "";
  int     iNumImages    = 1;
  double  fExposureTime = 0.0001;
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
  int     iTriggerMode  = 0;
  bool    bTriggerFast  = false;
  bool    bCropMode     = false;
  int     iOutputMode   = 0;
  int     iMenu         = 0;

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
            char* pNextToken = optarg;
            iBinX = strtoul(pNextToken, &pNextToken, 0); ++pNextToken;
            if ( *pNextToken == 0 ) break;
            iBinY = strtoul(pNextToken, &pNextToken, 0); ++pNextToken;
            break;
          }
      case 'i':
          iTriggerMode    = strtoul(optarg, NULL, 0);
          break;
      case 'f':
          bTriggerFast    = true;
          break;
      case 'x':
          bCropMode       = true;
          break;
      case 'o':
          iOutputMode     = strtoul(optarg, NULL, 0);
          break;
      case 'm':
          iMenu = 1;
          break;
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
    fTemperature, iRoiX, iRoiY, iRoiW, iRoiH, iBinX, iBinY, iTriggerMode, sFnPrefix, iNumImages,
    iOutputMode, iMenu, bTriggerFast, bCropMode);

  int iError = testCamera.init();
  if (iError == 0)
  {
    iError = testCamera.run();
  }

  testCamera.deinit();
  return 0;
}
