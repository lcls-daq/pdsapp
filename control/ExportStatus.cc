// $Id$

#include "ExportStatus.hh"
#include "RunStatus.hh"
#include "ConfigSelect.hh"
#include "StateSelect.hh"

#include "pds/service/Task.hh"
#include "pds/service/TaskObject.hh"

#include <strings.h>

using namespace Pds;

//
// ExportStatus
//
ExportStatus::ExportStatus(RunStatus *runStatus, ConfigSelect *configSelect, StateSelect *stateSelect, unsigned status_port) :
  _task         (new Task(TaskObject("sndsta"))),
  _runStatus    (runStatus),
  _configSelect (configSelect),
  _stateSelect  (stateSelect),
  _period       (1),
  _status_port  (status_port),
  _sem          (Semaphore::FULL),
  _configured   (false),
  _running      (false)
{
  /* open socket */
  _sendFd = socket(AF_INET,SOCK_DGRAM, 0);
  if (_sendFd == -1) {
    perror("socket");
  }

  /* init _sendAddr */
  bzero(&_sendAddr, sizeof(_sendAddr));
  _sendAddr.sin_family = AF_INET;
  _sendAddr.sin_addr.s_addr=inet_addr("127.0.0.1");
  _sendAddr.sin_port=htons(_status_port);
}

int ExportStatus::send(char *msg)
{
  int rv = 1;
  int sent = sendto(_sendFd, (void *)msg, strlen(msg), 0,
             (struct sockaddr *)&_sendAddr, sizeof(_sendAddr));
  if (sent == -1) {
    perror("sendto");
  } else {
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
             _runStatus->runNumber(), duration, bytes, events, damaged, _controlState.c_str());

  } else {
   
    snprintf(localbuf, sizeof(localbuf), UPDATE_2_FORMAT,
             _configured ? 1 : 0, _configSelect->getType().c_str(), _stateSelect->record_state() ? 1 : 0, _controlState.c_str());
  }

  if (strlen(localbuf) > 0) {
    send(localbuf);
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
