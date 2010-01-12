#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <pthread.h> 
#include <unistd.h>
#include "pvcam/include/master.h"
#include "pvcam/include/pvcam.h"

int testPIDaq();

namespace PICAM
{
  void printPvError(const char *sPrefixMsg);
  void displayParamIdInfo(int16 hCam, uns32 uParamId,
                          const char sDescription[]);
  int getAnyParam(int16 hCam, uns32 uParamId, void *pParamValue);
  int setAnyParam(int16 hCam, uns32 uParamId, void *pParamValue);
  
  void printROI(int iNumRoi, rgn_type* roi);
}

using PICAM::printPvError;

class ImageCapture
{
public:
  int start();    
  int getConfig(int& x, int& y, int& iWidth, int& iHeight, int& iBinning);
  int lockCurrentFrame(unsigned char*& pCurrentFrame);
  int unlockFrame();
  
private:   
  // private functions
  int testImageCaptureStandard(int16 hCam, int iNumFrame, int16 modeExposure, uns32 iExposureTime);
  int testImageCaptureContinous(int16 hCam, int iNumFrame, int16 modeExposure, uns32 iExposureTime);  
  int setupROI(int16 hCam, rgn_type& region);

  // private data
  unsigned char* _pCurrentFrame;
  unsigned char* _pLockedFrame;
  bool _bLockFrame;
  
  // private static functions
  static int displayCameraSettings(int16 hCam);
  static int setupCooling(int16 hCam);  
};

class TestDaqThread
{
public:
  int start();
  
private:
  static int createSendDataThread();
  static void *sendDataThread(void *);  
};

int main(int argc, char **argv)
{
  testPIDaq();
}

#include <signal.h>

int testPIDaq()
{
  ImageCapture  imageCapure;
  TestDaqThread testDaqThread;    
  
  int iFail = testDaqThread.start();
  if (0 != iFail)
  {    
    printf("testPIDaq(): testDaqThread.start() failed, error code = %d\n", iFail);
    return 1;
  }
  
  iFail = imageCapure.start();
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

int ImageCapture::displayCameraSettings(int16 hCam)
{
  using PICAM::displayParamIdInfo;
  displayParamIdInfo(hCam, PARAM_EXPOSURE_MODE, "Exposure Mode");
  displayParamIdInfo(hCam, PARAM_CLEAR_MODE, "Clear Mode");
  displayParamIdInfo(hCam, PARAM_SHTR_OPEN_MODE, "Shutter Open Mode");
  displayParamIdInfo(hCam, PARAM_SHTR_OPEN_DELAY, "Shutter Open Delay");
  displayParamIdInfo(hCam, PARAM_SHTR_CLOSE_DELAY, "Shutter Close Delay");
  displayParamIdInfo(hCam, PARAM_EDGE_TRIGGER, "Edge Trigger" );
    
  displayParamIdInfo(hCam, PARAM_EXP_RES, "Exposure Resolution");
  displayParamIdInfo(hCam, PARAM_EXP_RES_INDEX, "Exposure Resolution Index");
  
  return 0;
}

int ImageCapture::setupCooling(int16 hCam)
{
  using namespace PICAM;
  // Cooling settings
  
  displayParamIdInfo(hCam, PARAM_COOLING_MODE, "Cooling Mode");
  //displayParamIdInfo(hCam, PARAM_TEMP_SETPOINT, "Set Cooling Temperature *Org*");  

  int16 iTemperatureSet = -1000;
  const int iMaxWaitingTime = 10000; // in miliseconds
  setAnyParam( hCam, PARAM_TEMP_SETPOINT, &iTemperatureSet );
  displayParamIdInfo( hCam, PARAM_TEMP_SETPOINT, "Set Cooling Temperature" );   

  displayParamIdInfo( hCam, PARAM_TEMP, "Temperature *Org*" );  
  
  timeval timeSleepMicroOrg = {0, 1000 }; // 1 milliseconds    
  timespec timeVal1;
  clock_gettime( CLOCK_REALTIME, &timeVal1 );      
  
  int16 iTemperatureCurrent = -1;  
  int iNumLoop = 0;
  /* wait for data or error */
  while (1)
  {  
    getAnyParam( hCam, PARAM_TEMP, &iTemperatureCurrent );
    if ( iTemperatureCurrent <= iTemperatureSet ) break;
    
    if ( (iNumLoop+1) % 1000 == 0 )
      displayParamIdInfo( hCam, PARAM_TEMP, "Temperature *Updating*" );
    
    if ( iNumLoop > iMaxWaitingTime ) break;
    
    // This data will be modified by select(), so need to be reset
    timeval timeSleepMicro = timeSleepMicroOrg; 
    // use select() to simulate nanosleep(), because experimentally select() controls the sleeping time more precisely
    select( 0, NULL, NULL, NULL, &timeSleepMicro);    
    iNumLoop++;
  }
  
  timespec timeVal2;
  clock_gettime( CLOCK_REALTIME, &timeVal2 );
  double fCoolingTime = (timeVal2.tv_nsec - timeVal1.tv_nsec) * 1.e-6 + ( timeVal2.tv_sec - timeVal1.tv_sec ) * 1.e3;    
  printf("Cooling Time = %6.3lf ms\n", fCoolingTime);  
  
  displayParamIdInfo( hCam, PARAM_TEMP, "Final Temperature" );
  
  return 0;
}

int ImageCapture::start()
{
  char cam_name[CAM_NAME_LEN];  /* camera name                    */
  int16 hCam;                   /* camera handle                  */

  rs_bool bStatus;

  /* Initialize the PVCam Library and Open the First Camera */
  bStatus = pl_pvcam_init();
  if (!bStatus)
  {
    printPvError("ImageCapture::start(): pl_pvcam_init() failed");
    return 1;
  }

  bStatus = pl_cam_get_name(0, cam_name);
  if (!bStatus)
  {
    printPvError("ImageCapture::start(): pl_cam_get_name(0) failed");
    return 2;
  }

  bStatus = pl_cam_open(cam_name, &hCam, OPEN_EXCLUSIVE);
  if (!bStatus)
  {
    printPvError("ImageCapture::start(): pl_cam_open() failed");
    return 3;
  }

  displayCameraSettings(hCam);
  
  setupCooling(hCam);
    
  const int iNumFrame = 5;
  const int16 modeExposure = TIMED_MODE;
  //const int16 modeExposure = STROBED_MODE;
  const uns32 iExposureTime = 1;
  
  //testImageCaptureStandard(hCam, iNumFrame, modeExposure, iExposureTime);
  testImageCaptureContinous(hCam, iNumFrame, modeExposure, iExposureTime);

  bStatus = pl_cam_close(hCam);
  if (!bStatus)
  {
    printPvError("ImageCapture::start(): pl_cam_close() failed");
    return 4;
  }

  bStatus = pl_pvcam_uninit();
  if (!bStatus)
  {
    printPvError("ImageCapture::start(): pl_pvcam_uninit() failed");
    return 5;
  }
   
  return 0;
}

int ImageCapture::setupROI(int16 hCam, rgn_type& region)
{
  using PICAM::getAnyParam;
  int iWidth, iHeight;
  getAnyParam( hCam, PARAM_SER_SIZE, &iWidth );
  getAnyParam( hCam, PARAM_PAR_SIZE, &iHeight );

  // set the region to use full frame and 1x1 binning
  region.s1 = 0;
  region.s2 = iWidth - 1;
  region.sbin = 1;
  region.p1 = 0;
  region.p2 = iHeight - 1;
  region.pbin = 1;

  return 0;
}

int ImageCapture::testImageCaptureStandard(int16 hCam, int iNumFrame, int16 modeExposure, uns32 iExposureTime)
{
  printf( "Starting standard image capture for %d frames, exposure mode = %d, exposure time = %d ms\n",
    iNumFrame, (int) modeExposure, (int) iExposureTime );
    
  rgn_type region;
  setupROI(hCam, region);
  PICAM::printROI(1, &region);
  
  /* Init a sequence set the region, exposure mode and exposure time */
  pl_exp_init_seq();
  
  uns32 uFrameSize = 0;
  pl_exp_setup_seq(hCam, 1, 1, &region, modeExposure, iExposureTime, &uFrameSize);
  uns16* pFrameBuffer = (uns16 *) malloc(uFrameSize);
  printf( "frame size for standard capture = %lu\n", uFrameSize );

  timeval timeSleepMicroOrg = {0, 1000 }; // 1 milliseconds  

  /* Start the acquisition */
  printf("Collecting %i Frames\n", iNumFrame);
  
  double fAvgStartupTime = 0, fAvgPollingTime = 0, fAvgReadoutTime = 0, fAvgSingleFrameTime = 0;  
  /* ACQUISITION LOOP */
  for (int iFrame = 0; iFrame < iNumFrame; iFrame++)
  {    
    /* frame now contains valid data */
    printf( "Taking frame %d...", iFrame );    
    
    timespec timeVal1;
    clock_gettime( CLOCK_REALTIME, &timeVal1 );    
    
    pl_exp_start_seq(hCam, pFrameBuffer);

    timespec timeVal2;
    clock_gettime( CLOCK_REALTIME, &timeVal2 );
    
    uns32 uNumBytesTransfered;
    int iNumLoop = 0;
    int16 status = 0;

    /* wait for data or error */
    while (pl_exp_check_status(hCam, &status, &uNumBytesTransfered) &&
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
      break;
    }    

    timespec timeVal3;
    clock_gettime( CLOCK_REALTIME, &timeVal3 );

    double fReadoutTime = -1;
    PICAM::getAnyParam(hCam, PARAM_READOUT_TIME, &fReadoutTime);
    
    double fStartupTime = (timeVal2.tv_nsec - timeVal1.tv_nsec) * 1.e-6 + ( timeVal2.tv_sec - timeVal1.tv_sec ) * 1.e3;    
    double fPollingTime = (timeVal3.tv_nsec - timeVal2.tv_nsec) * 1.e-6 + ( timeVal3.tv_sec - timeVal2.tv_sec ) * 1.e3;    
    double fSingleFrameTime = fStartupTime + fPollingTime;
    printf(" Startup Time = %6.3lf Polling Time = %7.3lf Readout Time = %.3lf Frame Time = %.3lf\n", 
      fStartupTime, fPollingTime, fReadoutTime, fSingleFrameTime );        
      
    fAvgStartupTime += fStartupTime; fAvgPollingTime += fPollingTime; fAvgReadoutTime += fReadoutTime; fAvgSingleFrameTime += fSingleFrameTime;
  }

  // pl_exp_finish_seq(hCam, pFrameBuffer, 0); // No need to call this function, unless we have multiple ROIs

  /*Uninit the sequence */
  pl_exp_uninit_seq();
 
  fAvgStartupTime /= iNumFrame; fAvgPollingTime /= iNumFrame; fAvgReadoutTime /= iNumFrame; fAvgSingleFrameTime /= iNumFrame;
  printf("Average Startup Time = %6.3lf Polling Time = %7.3lf Readout Time = %.3lf Frame Time = %.3lf\n", 
    fAvgStartupTime, fAvgPollingTime, fAvgReadoutTime, fAvgSingleFrameTime );        

  free(pFrameBuffer);
  
  return 0;
}

/* Circ Buff and App Buff
   to be able to store more frames than the circular buffer can hold */
int ImageCapture::testImageCaptureContinous(int16 hCam, int iNumFrame, int16 modeExposure, uns32 iExposureTime)
{
  printf( "Starting continuous image capture for %d frames, exposure mode = %d, exposure time = %d ms\n",
    iNumFrame, (int) modeExposure, (int) iExposureTime );
  
  const int16 iCircularBufferSize = 5;  
  const timeval timeSleepMicroOrg = {0, 1000}; // 1 milliseconds  
  
  rgn_type region;  
  setupROI(hCam, region);
  PICAM::printROI(1, &region);

  /* Init a sequence set the region, exposure mode and exposure time */
  if (!pl_exp_init_seq())
  {
    printPvError("ImageCapture::testImageCaptureContinous(): pl_exp_init_seq() failed!\n");
    return 2; 
  }
 
  uns32 uFrameSize = 0;
  //if( pl_exp_setup_cont( hCam, 1, region, TIMED_MODE, exp_time, &uFrameSize, CIRC_NO_OVERWRITE ) ) {
  if (!pl_exp_setup_cont(hCam, 1, &region, modeExposure, iExposureTime, &uFrameSize, CIRC_NO_OVERWRITE))
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
  if (!pl_exp_start_cont(hCam, pFrameBuffer, iBufferSize))
  {
    printPvError("ImageCapture::testImageCaptureContinous():pl_exp_start_cont() failed");
    free(pBufferWithHeader);
    return PV_FAIL;
  }

  double fAvgPollingTime = 0, fAvgFrameProcessingTime = 0, fAvgReadoutTime = 0, fAvgSingleFrameTime = 0;
  /* ACQUISITION LOOP */
  for (int iFrame = 0; iFrame < iNumFrame; iFrame++)
  {    
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
      if (!pl_exp_check_cont_status(hCam, &status, &uNumBytesTransfered, &uNumBufferFilled) )
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
    if (!pl_exp_get_oldest_frame(hCam, &pFrameCurrent))
    {
      printPvError("ImageCapture::testImageCaptureContinous():pl_exp_start_cont() failed");
      break;
    }    
    
    printf( "Current frame = %p\n", pFrameCurrent );
    
    for ( int iCopy = 0; iCopy < 1; iCopy++ )
    {
      memcpy( pBufferWithHeader+iHeaderSize/2, pBufferWithHeader, iHeaderSize/2 );
    }
    
    if ( !pl_exp_unlock_oldest_frame(hCam) ) 
    {
      printPvError("ImageCapture::testImageCaptureContinous():pl_exp_unlock_oldest_frame() failed");
      break;
    }
    
    timespec timeVal3;
    clock_gettime( CLOCK_REALTIME, &timeVal3 );
    

    double fReadoutTime = -1;
    PICAM::getAnyParam(hCam, PARAM_READOUT_TIME, &fReadoutTime);
    
    double fPollingTime = (timeVal2.tv_nsec - timeVal1.tv_nsec) * 1.e-6 + ( timeVal2.tv_sec - timeVal1.tv_sec ) * 1.e3;    
    double fFrameProcessingTime = (timeVal3.tv_nsec - timeVal2.tv_nsec) * 1.e-6 + ( timeVal3.tv_sec - timeVal2.tv_sec ) * 1.e3;    
    double fSingleFrameTime = fPollingTime + fFrameProcessingTime ;
    printf(" Polling Time = %7.3lf Frame Processing Time = %.3lf Readout Time = %.3lf Frame Time = %.3lf\n", 
      fPollingTime, fFrameProcessingTime, fReadoutTime, fSingleFrameTime );        
    
    fAvgPollingTime += fPollingTime; fAvgFrameProcessingTime += fFrameProcessingTime; fAvgReadoutTime += fReadoutTime; fAvgSingleFrameTime += fSingleFrameTime;
  }

  /* Stop the acquisition */
  if (!pl_exp_stop_cont(hCam, CCS_HALT)) printPvError("ImageCapture::testImageCaptureContinous():pl_exp_stop_cont() failed");
  
  // pl_exp_finish_seq(hCam, pFrameBuffer, 0); // No need to call this function, unless we have multiple ROIs

  /* Uninit the sequence */
  if (!pl_exp_uninit_seq()) printPvError("ImageCapture::testImageCaptureContinous():pl_exp_uninit_seq() failed");
  
  free(pBufferWithHeader);
    
  fAvgPollingTime /= iNumFrame; fAvgFrameProcessingTime /= iNumFrame; fAvgReadoutTime /= iNumFrame; fAvgSingleFrameTime /= iNumFrame;
  printf("Averge Polling Time = %7.3lf Frame Processing Time = %.3lf Readout Time = %.3lf Frame Time = %.3lf\n", 
      fAvgPollingTime, fAvgFrameProcessingTime, fAvgReadoutTime, fAvgSingleFrameTime );        

  return PV_OK;
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
  boolean status, status2, status3, status4, status5; /* status of pvcam functions */

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
    default:
      printf(" data type not supported in this functions\n");
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

/* This is an example that will display information we can get from parameter id.      */
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

int getAnyParam(int16 hCam, uns32 uParamId, void *pParamValue)
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

  bStatus = pl_get_param(hCam, uParamId, ATTR_CURRENT, pParamValue);
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

}                               // namespace PICAM
