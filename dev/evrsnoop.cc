#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <signal.h>
#include <memory.h>

#include "evgr/evr/evr.hh"
#include "pds/evgr/EvgrBoardInfo.hh"

using namespace Pds;

static EvgrBoardInfo<Evr> *erInfoGlobal;

class EvrStandAloneManager {
public:
  EvrStandAloneManager(EvgrBoardInfo<Evr>& erInfo, char opcodes[]);
  void configure();
  void start();
  void stop();
private:
  Evr& _er;
  char *pOpcodes;
};

void EvrStandAloneManager::start() {
  unsigned ram=0;
  _er.MapRamEnable(ram,1);
};

void EvrStandAloneManager::stop() { 
  _er.IrqEnable(0);
  _er.Enable(0);
  _er.EnableFIFO(0);  
}

void EvrStandAloneManager::configure() {
  printf("Configuring evr\n");
  //_er.Reset();

  //int pulse = 0; int presc = 1; int enable = 1;
  //int polarity=0;  int map_reset_ena=0; int map_set_ena=0; int map_trigger_ena=1;
  //_er.SetPulseProperties(pulse, polarity, map_reset_ena, map_set_ena, map_trigger_ena,
  //                         enable);
  //const double tickspersec = 119.0e6;

  //int delay=(int)(0.0*tickspersec);
  //int width=(int)(100.e-6*tickspersec);
  //_er.SetPulseParams(pulse,presc,delay,width);

  //_er.SetUnivOutMap( 8, pulse);
  //_er.SetUnivOutMap( 9, pulse);

  int ram=0;   
  
  for ( int opcode=0; opcode<=255; opcode++ )
  {
    if ( pOpcodes[opcode] == 0 )
      continue;
      
    int enable = 1;
    _er.SetFIFOEvent(ram, opcode, enable);
    //int trig=0; int set=-1; int clear=-1;
    //_er.SetPulseMap(ram, opcode, trig, set, clear);
  }
      
  // setup map ram
  _er.MapRamEnable(ram,0);
  
  _er.IrqEnable(EVR_IRQ_MASTER_ENABLE | EVR_IRQFLAG_EVENT);
  _er.EnableFIFO(1);
  _er.Enable(1);
}

extern "C" {
  void evrsa_sig_handler(int parm)
  {
    Evr& er = erInfoGlobal->board();
    FIFOEvent fe;
    while( !er.GetFIFOEvent(&fe) ) {
      printf("Received Fiducial %06x  Event code %03d  Timestamp %d\n", fe.TimestampHigh, fe.EventCode, fe.TimestampLow);
    }
    int fdEr = erInfoGlobal->filedes();
    er.IrqHandled(fdEr);
  }
}

EvrStandAloneManager::EvrStandAloneManager(EvgrBoardInfo<Evr> &erInfo, char opcodes[]) :
  _er(erInfo.board()), pOpcodes(opcodes)
{
  configure();

  erInfoGlobal = &erInfo;
  _er.IrqAssignHandler(erInfo.filedes(), &evrsa_sig_handler);
  
  start();  
}

static EvrStandAloneManager*  pEvrStandAloneManager = NULL;
static bool                   bProgramStop          = false;

void evrStandAloneSignalHandler( int iSignalNo )
{
  printf( "\nevrStandAloneSignalHandler(): signal %d received.\n", iSignalNo );  
  
  //if ( pEvrStandAloneManager ) 
  //  pEvrStandAloneManager->stop();
    
  bProgramStop = true;
}

/*
 * Set the opcodes byte array, according to the input string
 *
 * Examples:
 *   1,2,3,4  ->  Set opcodes 1,2,3 and 4
 *   1-4      ->  Set opcodes from 1 to 4
 *   1,2-4,5  ->  Set opcodes 1, from 2 to 4, and 5
 */
void selectOpcodes( char opcodes[256], char* strSelection )
{  
  enum EnumPrevOperation
  {
    EnumPrevOpNewNumber,
    EnumPrevOpRangeTo,
  } enumPrevOp = EnumPrevOpNewNumber;  
    
  int prevNumber = -1;
  int currNumber = -1;
  for ( char* pc = strSelection; *pc != 0; pc++ )
  {
    if ( *pc == ' ' || *pc == '\t' )
      continue;
      
    if ( *pc == ',' )
    {
      if ( currNumber >= 0 && currNumber <= 255  )
      {
        if ( enumPrevOp == EnumPrevOpNewNumber )
        {
          printf( "Set opcode [%d]\n", currNumber );
          opcodes[currNumber] = 1;
        }
        else // ( enumPrevOp == EnumPrevOpRangeTo )
        {
          printf( "Set opcode [%d - %d]\n", prevNumber, currNumber );          
          for ( int number = prevNumber; number <= currNumber; number++ )
            opcodes[number] = 1;
        }
      }
      
      currNumber = prevNumber = -1;
      enumPrevOp = EnumPrevOpNewNumber;
    }
    else if ( *pc == '-' )
    {
      if ( currNumber >= 0 && currNumber <= 255  )
      {
        prevNumber = currNumber;
        currNumber = -1;
        enumPrevOp = EnumPrevOpRangeTo;      
      }
      else
      {
        currNumber = prevNumber = -1;
        enumPrevOp = EnumPrevOpNewNumber;      
      }      
    }
    else if ( *pc >= '0' && *pc <= '9' )
    {
      int digit = *pc - '0';
      if ( currNumber < 0 )
        currNumber = digit;
      else
        currNumber = currNumber * 10 + digit;
    }       
    
  } // for ( char* pc = strSelection; pc != NULL; pc++ )

  if ( currNumber >= 0 && currNumber <= 255  )
  {  
    if ( enumPrevOp == EnumPrevOpNewNumber )
    {
      printf( "Set opcode [%d]\n", currNumber );
      opcodes[currNumber] = 1;
    }
    else // ( enumPrevOp == EnumPrevOpRangeTo )
    {
      printf( "Set opcode [%d - %d]\n", prevNumber, currNumber );          
      for ( int number = prevNumber; number <= currNumber; number++ )
        opcodes[number] = 1;
    }
  }
  
  return;
}

void showUsage()
{
  printf( "Usage:  evrstandalone  [-h] [-r <a/b/c/d>] [-o <event list>]"
    "  Options:\n"
    "    -h               Show usage\n"
    "    -r <a/b/c/d>     Use evr device a/b/c/d\n"
    "    -o <event list>  Select event codes\n"
    " ------------------------------------------------\n"
    " Event List Examples:\n"
    "   -o 1,2,3,4    ->  Select opcodes 1,2,3 and 4\n"
    "   -o 1-4        ->  Select opcodes from 1 to 4\n"
    "   -o 1,2-4,6-8  ->  Select opcodes 1, from 2 to 4, and from 6-8\n"
  );
}

int main(int argc, char** argv) {

  extern char* optarg;
  char* evrid=0;
  char  opcodes[256];
  
  memset( opcodes, 0, 256 );
  
  int c;
  while ( (c=getopt( argc, argv, "r:o:h")) != EOF ) {
    switch(c) {
    case 'r':
      evrid  = optarg;
      break;
    case 'o':
      selectOpcodes( opcodes, optarg );
      break;
    case 'h':
      showUsage();
      return 0;
    }
  }

  char defaultdev='a';
  if (!evrid) evrid = &defaultdev;
  
  /*
   * Register singal handler
   */
  struct sigaction sigActionSettings;
  sigemptyset(&sigActionSettings.sa_mask);
  sigActionSettings.sa_handler = evrStandAloneSignalHandler;
  sigActionSettings.sa_flags   = SA_RESTART;    

  if (sigaction(SIGINT, &sigActionSettings, 0) != 0 ) 
    printf( "main(): Cannot register signal handler for SIGINT\n" );
  if (sigaction(SIGTERM, &sigActionSettings, 0) != 0 ) 
    printf( "main(): Cannot register signal handler for SIGTERM\n" );
  
  char evrdev[16];
  sprintf(evrdev,"/dev/er%c3",*evrid);
  printf("Using evr %s\n",evrdev);
  printf("Press Ctrl + C to exit the program...\n" );

  EvgrBoardInfo<Evr>& erInfo = *new EvgrBoardInfo<Evr>(evrdev);
  pEvrStandAloneManager = new EvrStandAloneManager(erInfo, opcodes);
  
  do
  {
    pause(); // wait for any signal  
  }
  while ( ! bProgramStop );
  
  delete pEvrStandAloneManager;
  return 0;
}
