// $Id$

#include "ExportStatus.hh"
#include "RunStatus.hh"
#include "ConfigSelect.hh"
#include "StateSelect.hh"

#include "pds/service/Task.hh"
#include "pds/service/TaskObject.hh"

#include <strings.h>
#include <string.h>
#include <stdlib.h>

// forward declaration 
static int open_udp_socket(const char *host_and_port, bool verbose);

using namespace Pds;

//
// ExportStatus
//
ExportStatus::ExportStatus(RunStatus *runStatus, ConfigSelect *configSelect, StateSelect *stateSelect, const char *host_and_port, const char *partition) :
  _task         (new Task(TaskObject("sndsta"))),
  _runStatus    (runStatus),
  _configSelect (configSelect),
  _stateSelect  (stateSelect),
  _period       (1),
  _sem          (Semaphore::FULL),
  _configured   (false),
  _running      (false)
{
  /* open socket */
  _sendFd = open_udp_socket(host_and_port, false);
  if (_sendFd == -1) {
    fprintf(stderr, "%s: open_udp_socket() failed\n", __FUNCTION__);
  }

  /* init station from a string like "CXI:1" */
  char *pColon = strchr(partition, ':');
  if (pColon) {
    _station = atoi(pColon+1);
  } else {
    _station = 0;
  }
}

int ExportStatus::mysend(char *msg)
{
  int rv = 1;
  if (_sendFd != -1) {
    (void) ::send(_sendFd, (void *)msg, strlen(msg), 0);
    rv = 0;   // OK
  }
  return (rv);
}

Task* ExportStatus::task() { return _task; }
unsigned ExportStatus::duration() const { return (unsigned) (1000 * _period); } // msecs
unsigned ExportStatus::repetitive() const { return 1; }

void ExportStatus::expired()
{
  char localbuf[512];

  if (_running) {
    unsigned long long duration, events, damaged, bytes;

    _runStatus->get_counts(&duration, &events, &damaged, &bytes);

    snprintf(localbuf, sizeof(localbuf), UPDATE_1_FORMAT,
             _runStatus->runNumber(), duration, bytes / (1024 * 1024), events, damaged, _station, _controlState.c_str());

  } else {
   
    snprintf(localbuf, sizeof(localbuf), UPDATE_2_FORMAT,
             _configured ? 1 : 0, _configSelect->getType().c_str(), _stateSelect->record_state() ? 1 : 0, _station, _controlState.c_str());
  }

  if (strlen(localbuf) > 0) {
    mysend(localbuf);
  }
}

void ExportStatus::configured(bool v)
{
  _configured = v;
}

void ExportStatus::change_state(QString ss)
{
  _controlState = ss.toStdString();

  if (ss == QString(TransitionId::name(TransitionId::BeginRun))) {
    _running = true;
  }
  if (ss == QString(TransitionId::name(TransitionId::EndRun  ))) {
    _running = false;
  }
}

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

static int open_udp_socket(const char *host_and_port, bool verbose)
{
    struct addrinfo hints;
    struct addrinfo *result, *rp;
    int sfd, s;
    char *portbuf = NULL;
    char hostbuf[40];

    if (verbose) {
      printf("entered %s(\"%s\", %s)\n", __FUNCTION__, host_and_port, verbose ? "true" : "false");
    }

    // parse host/port
    char *pColon = strchr(host_and_port, ':');
    if (pColon && ((pColon - host_and_port) < (int)sizeof(hostbuf))) {
      memset(hostbuf, 0, sizeof(hostbuf));
      strncpy(hostbuf, host_and_port, pColon - host_and_port);
      portbuf = pColon+1;
    } else {
      fprintf(stderr, "%s: invalid <host>:<port>: '%s'\n", __FUNCTION__, host_and_port);
      return (-1);        // ERROR
    }

    // Obtain address(es) matching host/port

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;    // Allow IPv4 or IPv6
    hints.ai_socktype = SOCK_DGRAM; // DGRAM socket
    hints.ai_flags = 0;
    hints.ai_protocol = 0;          // Any protocol

    if (verbose) {
      printf("%s: hostbuf=\"%s\" portbuf=\"%s\"\n", __FUNCTION__, hostbuf, portbuf); 
    }
    s = getaddrinfo(hostbuf, portbuf, &hints, &result);
    if (s != 0) {
        fprintf(stderr, "%s: getaddrinfo: %s\n", __FUNCTION__, gai_strerror(s));
        return (-1);  // ERROR
    }

    // getaddrinfo() returns a list of address structures
    // If socket(2) fails, try the next address.
    // If connect(2) fails, close then try the next address.

    for (rp = result; rp != NULL; rp = rp->ai_next) {
        sfd = socket(rp->ai_family, rp->ai_socktype,
                     rp->ai_protocol);
        if (sfd == -1) {
            perror("socket");
            continue;       // Failure
        }

        if (connect(sfd, rp->ai_addr, rp->ai_addrlen) != -1) {
            if (verbose) {
              printf("%s: connect() succeeded\n", __FUNCTION__);
            }
            break;          // Success
        } else {
            close(sfd);
        }
    }

    if (rp == NULL) {               // No address succeeded
        fprintf(stderr, "%s: Could not open socket\n", __FUNCTION__);
        return (-1);        // ERROR
    }

    freeaddrinfo(result);   // No longer needed

    return (sfd);
}
