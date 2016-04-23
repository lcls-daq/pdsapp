/**
 **  Executable to operate LCLS-II timing receiver and Keysight ADC together
 **    Record data to SSD.
 **/

#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <time.h>
#include <semaphore.h>

#include "pds/service/CmdLineTools.hh"
#include "pdsapp/test/Timing.hh"

#include <string>
#include <vector>

extern int optind;

using namespace Pds;

static int fd_evr_read;
static int fd_evr_free;
static int fd_adc_read;
static int fd_adc_free;

struct read_evr_args_s {
  int fd;
};

struct read_adc_args_s {
  sem_t* sem_block;
};
  
static void* read_evr(void*);
static void* read_adc(void*);

static const unsigned NCHANNELS=12;

void usage(const char* p) {
  printf("Usage: %s -r <a/b> -R <rate> [-I] [-f <input file> | -p<channel,delay,width,polarity]> -o <filename>\n",p);
  printf("\t<rate>: {0=1MHz, 1=0.5MHz, 2=100kHz, 3=10kHz, 4=1kHz, 5=100Hz, 6=10Hz, 7=1Hz}\n");
}

int main(int argc, char** argv) {

  extern char* optarg;
  char evrid='a';
  std::vector<unsigned> channel;
  std::vector<unsigned> delay;
  std::vector<unsigned> width;
  std::vector<unsigned> polarity;
  TprBase::FixedRate rate = TprBase::_1K;
  bool lInternal=false;
  const char* filename = 0;

  char* endptr;
  int c;
  bool lUsage = false;
  bool parseOK;
  while ( (c=getopt( argc, argv, "Ir:o:f:p:R:h?")) != EOF ) {
    switch(c) {
    case 'r':
      evrid  = optarg[0];
      if (strlen(optarg) != 1) {
        printf("%s: option `-r' parsing error\n", argv[0]);
        lUsage = true;
      }
      break;
    case 'I':
      lInternal=true;
      break;
    case 'o':
      filename = optarg;
      break;
    case 'f':
      { FILE* f = fopen(optarg,"r");
        if (f) {
          size_t sz=4096;
          char * line = new char[sz];
          while(getline(&line,&sz,f)>=0) {
            if (line[0]!='#') {
              channel .push_back(strtoul(line    ,&endptr,0));
              delay   .push_back(strtoul(endptr+1,&endptr,0));
              width   .push_back(strtoul(endptr+1,&endptr,0));
              polarity.push_back(strtoul(endptr+1,&endptr,0));
            }
          }
          delete[] line;
        }
        else {
          perror("Failed to open file");
        }
      }
      break;
    case 'p':
      channel .push_back(strtoul(optarg  ,&endptr,0));
      delay   .push_back(strtoul(endptr+1,&endptr,0));
      width   .push_back(strtoul(endptr+1,&endptr,0));
      polarity.push_back(strtoul(endptr+1,&endptr,0));
      break;
    case 'R':
      rate = (TprBase::FixedRate)strtoul(optarg,NULL,0);
      break;
    case 'h':
      usage(argv[0]);
      exit(0);
    case '?':
    default:
      lUsage = true;
      break;
    }
  }

  if (optind < argc) {
    printf("%s: invalid argument -- %s\n",argv[0], argv[optind]);
    lUsage = true;
  }

  if (lUsage) {
    usage(argv[0]);
    exit(1);
  }

  {
    char evrdev[16];
    sprintf(evrdev,"/dev/er%c3",evrid);
    printf("Using evr %s\n",evrdev);

    int fd = open(evrdev, O_RDWR);
    if (fd<0) {
      perror("Could not open");
      return -1;
    }

    void* ptr = mmap(0, sizeof(EvrReg), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
    if (ptr == MAP_FAILED) {
      perror("Failed to map");
      return -2;
    }

    EvrReg* p = reinterpret_cast<EvrReg*>(ptr);

    printf("SLAC Version[%p]: %08x\n", 
	   &(p->evr),
	   ((volatile uint32_t*)(&p->evr))[0x30>>2]);

    p->evr.IrqEnable(0);

    printf("Axi Version [%p]: BuildStamp: %s\n", 
	   &(p->version),
	   p->version.buildStamp().c_str());
    
    if (lInternal) {
      p->xbar.setTpr(XBar::LoopIn);
      p->xbar.setTpr(XBar::StraightOut);
    }
    else {
      p->xbar.setTpr(XBar::StraightIn);
      p->xbar.setTpr(XBar::LoopOut); 
   }
    p->xbar.dump();
  }

  {
    sem_t sem_block;
    sem_init(&sem_block,0,0);
    struct read_adc_args_s* args = new read_adc_args_s;
    args->sem_block = &sem_block;
    pthread_attr_t tattr;
    pthread_attr_init(&tattr);
    pthread_t tid;
    if (pthread_create(&tid, &tattr, &read_adc, args))
      perror("Error creating adc thread");
    sem_wait(&sem_block);
  }
  
  {
    char evrdev[16];
    sprintf(evrdev,"/dev/er%c3_1",evrid);
    printf("Using tpr %s\n",evrdev);

    int fd = open(evrdev, O_RDWR);
    if (fd<0) {
      perror("Could not open");
      return -1;
    }

    {
      struct read_evr_args_s* args = new read_evr_args_s;
      args->fd       = fd;
      { 
        pthread_attr_t tattr;
        pthread_attr_init(&tattr);
        pthread_t tid;
        if (pthread_create(&tid, &tattr, &read_evr, args))
          perror("Error creating read thread");
      }
    }

    void* ptr = mmap(0, sizeof(TprReg), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
    if (ptr == MAP_FAILED) {
      perror("Failed to map");
      return -2;
    }

    TprReg* p = reinterpret_cast<TprReg*>(ptr);
    
    p->tpr.rxPolarity(!lInternal);
    p->tpr.resetCounts();

    p->base.trigMaster=1;
    p->base.setupChannel(1, TprBase::Any, rate,
                         0, 0, 1);
    for(unsigned i=0; i<channel.size(); i++) {
      printf("Configure trigger %d for delay %d  width %d  polarity %d\n",
             channel[i],delay[i],width[i],polarity[i]);
      p->base.setupTrigger(channel[i],1,polarity[i],delay[i],width[i]);
    }
    p->base.dump();
  }

  sem_t block;
  sem_init(&block,0,0);
  sem_wait(&block);

  return 0;
}

void* read_evr(void* arg)
{
  struct read_evr_args_s* args = 
    reinterpret_cast<struct read_evr_args_s*>(arg);
  int         fd       = args->fd;

  uint32_t* data = new uint32_t[1024];
  
  EvrRxDesc* desc = new EvrRxDesc;
  desc->maxSize = 1024;
  desc->data    = data;
  
  unsigned eventFrames=0;
  unsigned dropFrames=0;

  ssize_t nb;
  while( (nb = read(fd, desc, sizeof(*desc))) >= 0 ) {
    uint64_t* p = (uint64_t*)data;
    
    char m = ' ';
    if (p[0]&(1ULL<<30)) {
      m = 'D';
      dropFrames++;
    }

    switch((p[0]>>32)&0xffff) {
    case 0:
      //
      //  Event DMA contains:
      //    32b: size (bits)
      //    16b: EventTag (0)
      //    16b: channel mask 
      //     xx: timing frame without BSA
      //
      eventFrames++;
      if (false)
	printf("EVENT  [x%llx]: %16llx %16llx %16llx %16llx %16llx %c\n",
	       (p[0]>>48)&0xffff,p[0],p[1],p[2],p[3],p[4],m);
      break;
    case 1: // BSA Control
    case 2: // BSA Frames
      break;
    default:
      printf("Unexpected DMA type [x%llx]\n",(p[0]>>32)&0xffff);
      break;
    }
  }

  return 0;
}

#include "AgMD2.h"
#include <vector>
using std::vector;
#include <unistd.h> // for usleep
#include <iostream>
using std::cout;
using std::cerr;
using std::hex;
using std::string;
#include <stdexcept>
using std::runtime_error;

static ViChar resource[] = "PXI2::0::0::INSTR";
static ViChar options[]  = "Simulate=false, DriverSetup= Model=U5303A";

#define checkApiCall( f ) do { ViStatus s = f; testApiCall( s, #f ); } while( false )

double triggertime = 10.e-6;
int numRecords = 50;
int recordSize = 10000;
double sampleRate = 1.6e9;
double delay = 0.0;
ViInt32 nbloop = 100;

// Utility function to check status error during driver API call.
void testApiCall( ViStatus status, char const * functionName )
{
    ViInt32 ErrorCode;
    ViChar ErrorMessage[256];

    if( status>0 ) // Warning occurred.
    {
        AgMD2_GetError( VI_NULL, &ErrorCode, sizeof(ErrorMessage), ErrorMessage );
        cerr << "** Warning during " << functionName << ": 0x" << hex << ", " << ErrorMessage << "\n";

    }
    else if( status<0 ) // Error occurred.
    {
        AgMD2_GetError( VI_NULL, &ErrorCode, sizeof(ErrorMessage), ErrorMessage );
        cerr << "** ERROR during " << functionName << ": 0x" << hex << ", " << ErrorMessage << "\n";
        throw runtime_error( ErrorMessage );
    }
}

void* read_adc(void* arg)
{
  struct read_adc_args_s* args = 
    reinterpret_cast<struct read_adc_args_s*>(arg);

    cout << "TSR acquisitions\n\n";

    // Initialize the driver. See driver help topic "Initializing the IVI-C Driver" for additional information.
    ViSession session;
    ViBoolean const idQuery = VI_FALSE;
    ViBoolean const reset   = VI_TRUE;
    checkApiCall( AgMD2_InitWithOptions( resource, idQuery, reset, options, &session ) );

    cout << "Driver initialized \n";

    // Read and output a few attributes.
    ViChar str[128];
    checkApiCall( AgMD2_GetAttributeViString( session, "", AGMD2_ATTR_SPECIFIC_DRIVER_PREFIX,               sizeof(str), str ) );
    cout << "Driver prefix:      " << str << "\n";
    checkApiCall( AgMD2_GetAttributeViString( session, "", AGMD2_ATTR_SPECIFIC_DRIVER_REVISION,             sizeof(str), str ) );
    cout << "Driver revision:    " << str << "\n";
    checkApiCall( AgMD2_GetAttributeViString( session, "", AGMD2_ATTR_SPECIFIC_DRIVER_VENDOR,               sizeof(str), str ) );
    cout << "Driver vendor:      " << str << "\n";
    checkApiCall( AgMD2_GetAttributeViString( session, "", AGMD2_ATTR_SPECIFIC_DRIVER_DESCRIPTION,          sizeof(str), str ) );
    cout << "Driver description: " << str << "\n";
    checkApiCall( AgMD2_GetAttributeViString( session, "", AGMD2_ATTR_INSTRUMENT_MODEL,                     sizeof(str), str) );
    cout << "Instrument model:   " << str << "\n";
    checkApiCall( AgMD2_GetAttributeViString( session, "", AGMD2_ATTR_INSTRUMENT_INFO_OPTIONS,              sizeof(str), str) );
    cout << "Instrument options: " << str << '\n';
    string const moduleOptions = str;
    checkApiCall( AgMD2_GetAttributeViString( session, "", AGMD2_ATTR_INSTRUMENT_FIRMWARE_REVISION,         sizeof(str), str ) );
    cout << "Firmware revision:  " << str << "\n";
    checkApiCall( AgMD2_GetAttributeViString( session, "", AGMD2_ATTR_INSTRUMENT_INFO_SERIAL_NUMBER_STRING, sizeof(str), str ) );
    cout << "Serial number:      " << str << "\n";

    ViBoolean simulate;
    checkApiCall( AgMD2_GetAttributeViBoolean( session, "", AGMD2_ATTR_SIMULATE, &simulate ) );
    cout << "\nSimulate:           " << ( simulate?"True":"False" ) << "\n";

    if( moduleOptions.find( "TSR" )==string::npos )
    {
        cerr << "The required TSR module option is missing from the instrument.\n";
        exit(1);
    }

    // Configure the acquisition.
    ViReal64 const range = 1.0;
    ViReal64 const offset = 0.0;
    ViInt32 const coupling = AGMD2_VAL_VERTICAL_COUPLING_DC;
    cout << "\nConfiguring acquisition\n";
    cout << "Range:              " << range << "\n";
    cout << "Offset:             " << offset << "\n";
    cout << "Coupling:           " << ( coupling?"DC":"AC" ) << "\n";
    checkApiCall( AgMD2_ConfigureChannel( session, "Channel1", range, offset, coupling, VI_TRUE ) );
    cout << "Number of records:  " << numRecords << "\n";
    cout << "Record size:        " << recordSize << "\n";
    checkApiCall( AgMD2_SetAttributeViInt64(   session, "", AGMD2_ATTR_NUM_RECORDS_TO_ACQUIRE, numRecords ) );
    checkApiCall( AgMD2_SetAttributeViInt64(   session, "", AGMD2_ATTR_RECORD_SIZE,            recordSize ) );

    checkApiCall( AgMD2_SetAttributeViBoolean( session, "", AGMD2_ATTR_TSR_ENABLED,            true       ) );

    // Configure the trigger.
    cout << "\nConfiguring trigger\n";
    checkApiCall( AgMD2_ConfigureEdgeTriggerSource( session, "External1", 1.0, AGMD2_VAL_POSITIVE) );
    checkApiCall( AgMD2_SetAttributeViInt32( session, "External1", AGMD2_ATTR_TRIGGER_COUPLING, AGMD2_VAL_TRIGGER_COUPLING_DC ) );
    checkApiCall( AgMD2_SetAttributeViString( session, "", AGMD2_ATTR_ACTIVE_TRIGGER_SOURCE, "External1" ) );
    { char name[32];
      AgMD2_GetAttributeViString( session, "", AGMD2_ATTR_ACTIVE_TRIGGER_SOURCE, 32, name );
      printf("Trigger source is [%s]\n",name);
    }
  
    // Calibrate the instrument.
    cout << "\nPerforming self-calibration\n";
    checkApiCall( AgMD2_SelfCalibrate( session ) );

    // Starting the endless acquisition loop.
    cout << "\nPerforming acquisition\n";
    ViInt64 arraySize = 0;
    checkApiCall( AgMD2_QueryMinWaveformMemory( session, 16, numRecords, 0, recordSize, &arraySize ) );

    if (recordSize<2000) arraySize = 2 * arraySize;
	
    vector<ViInt16> dataArray( arraySize );
    ViInt64 actualRecords = 0, waveformArrayActualSize = 0;
    ViInt64 actualPoints[numRecords], firstValidPoint[numRecords];
    ViReal64 initialXOffset[numRecords], initialXTimeSeconds[numRecords], initialXTimeFraction[numRecords];
    ViReal64 xIncrement = 0.0, scaleFactor = 0.0, scaleOffset = 0.0;
    ViInt64 numAcquiredRecord;	
    struct timespec tstart, tend;
    struct timespec tstart1, tend1;
    ViInt32 timeoutCounter = 10000;
    double readtime_cumul = 0.0;
    double readtime = 0.0;
    double initialXTimeDiff = 0.0;
    double initialXTime = 0.0;
    double initialXTimePrevious = 0.0;

    ViInt32 missed = 0;
    checkApiCall( AgMD2_InitiateAcquisition( session ) );
    clock_gettime(CLOCK_REALTIME, &tstart);

    sem_post(args->sem_block);

    for( int loop = 0; (1); loop++) {

	ViBoolean overflowOccurred = VI_FALSE;
        checkApiCall( AgMD2_GetAttributeViBoolean( session, "", AGMD2_ATTR_TSR_MEMORY_OVERFLOW_OCCURRED, &overflowOccurred ) );
        if( overflowOccurred!=VI_FALSE )
        {
            cout << loop << " - A memory overflow occurred while during TSR acquisition.\n";
 	    checkApiCall( AgMD2_Abort( session ) );
            sleep( 1 );
	    checkApiCall( AgMD2_InitiateAcquisition( session ) );
        }


       // Wait for enough records to be acquired, and check for overflow.
       ViInt32 timeoutCounter = 20000;
       ViBoolean isAcqComplete = VI_FALSE;
        while( !isAcqComplete && timeoutCounter-- )
        {
            checkApiCall( AgMD2_GetAttributeViBoolean( session, "", AGMD2_ATTR_TSR_IS_ACQUISITION_COMPLETE, &isAcqComplete ) );
            usleep( 100 );
        }
        if( !isAcqComplete )
        {
            cerr << "A timeout occurred while waiting for trigger during TSR acquisition.\n";
            break;
        }
  
          // Fetch the acquired data in array.
        clock_gettime(CLOCK_REALTIME, &tstart1);
        checkApiCall( AgMD2_FetchMultiRecordWaveformInt16( session, "Channel1", 0, numRecords, 0, recordSize, arraySize,
                      &dataArray[0], &waveformArrayActualSize, &actualRecords, actualPoints, firstValidPoint, initialXOffset,
                      initialXTimeSeconds, initialXTimeFraction, &xIncrement, &scaleFactor, &scaleOffset ) );
	clock_gettime(CLOCK_REALTIME, &tend1); 

 	readtime =  (tend1.tv_nsec - tstart1.tv_nsec) + (tend1.tv_sec-tstart1.tv_sec) * 1.e9;
//	readtime_cumul = readtime_cumul + readtime;

        printf("Loop %d:  actual records %d,  readtime %f\n",loop, actualRecords, readtime);

     // Release the corresponding memory and mark it as available for new acquisitions.
        checkApiCall( AgMD2_TSRContinue( session ) );
    }

    return 0;
}
