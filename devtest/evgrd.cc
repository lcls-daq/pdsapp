#include "pds/evgr/EvgrBoardInfo.hh"
#include "pds/evgr/EvgrManager.hh"

#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <syslog.h>
#include <string.h>
#include <stdio.h>

using namespace Pds;

int main(int argc, char** argv) 
{
  extern char* optarg;
  char* evgid=0;
  char* evrid=0;
  int c;
  while ( (c=getopt( argc, argv, "g:r:")) != EOF ) {
    switch(c) {
    case 'g':
      evgid = optarg;
      break;
    case 'r':
      evrid  = optarg;
      break;
    }
  }

  char defaultdev='a';
  if (!evrid) evrid = &defaultdev;
  if (!evgid) evgid = &defaultdev;

  char evgdev[16]; char evrdev[16];
  sprintf(evgdev,"/dev/eg%c3",*evgid);
  sprintf(evrdev,"/dev/er%c3",*evrid);

  // Fork off the parent process
  pid_t pid = fork();
  if (pid < 0) {
    exit(EXIT_FAILURE);
  }
  if (pid > 0) {
    exit(EXIT_SUCCESS); // exit parent
  }
  openlog("evgrd", LOG_PID, LOG_USER);
  syslog(LOG_INFO, "starting: evg device is %s, evr is %s", evgdev, evrdev);

  // Close out the standard file descriptors
  close(STDIN_FILENO);
  close(STDOUT_FILENO);
  close(STDERR_FILENO);
        
  // Create a new SID for the child process
  pid_t sid = setsid();
  if (sid < 0) {
    syslog(LOG_ERR, "setsid failed: %m");
    exit(EXIT_FAILURE);
  }

  EvgrBoardInfo<Evg>& egInfo = *new EvgrBoardInfo<Evg>(evgdev);
  EvgrBoardInfo<Evr>& erInfo = *new EvgrBoardInfo<Evr>(evrdev);
  EvgrManager* evgr = new EvgrManager(egInfo,erInfo);

  // Create the evgr manager and wait for signals
  sigset_t waitset;
  sigemptyset(&waitset);
  sigaddset(&waitset, SIGHUP);
  sigaddset(&waitset, SIGTERM);
  sigprocmask(SIG_BLOCK, &waitset, NULL);
  bool dorun = true;
  while (dorun) {
    int signo;
    int err = sigwait(&waitset, &signo);
    if (err != 0) {
      syslog(LOG_ERR, "sigwait failed");
      exit(EXIT_FAILURE);
    } 
    switch (signo) {
    case SIGHUP:
      syslog(LOG_INFO, "counted %d opcodes, last opcode value 0x%x", evgr->opcodecount(), evgr->lastopcode());
      break;
    case SIGTERM:
      dorun = false;
      break;
    default:
      syslog(LOG_INFO, "unexpected signal %d", signo);
      break;
    }
  }
  syslog(LOG_INFO, "exiting");
  exit(EXIT_SUCCESS);
}

