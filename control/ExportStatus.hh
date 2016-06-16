// $Id$

#ifndef Pds_ExportStatus_hh
#define Pds_ExportStatus_hh

#include <QtCore/QObject>

#include "pds/service/Timer.hh"
#include "pds/service/Semaphore.hh"
#include "pds/utility/Appliance.hh"

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string>

#define UPDATE_1_FORMAT               \
    "{"                               \
      "\"jsonrpc\": \"2.0\","         \
      "\"method\": \"update_1\","     \
      "\"params\": {"                 \
        "\"configured\" : 1,"         \
        "\"running\" : 1,"            \
        "\"run_number\" : %u,"        \
        "\"run_duration\" : %llu,"    \
        "\"run_mbytes\" : %llu,"      \
        "\"event_count\" : %llu,"     \
        "\"damage_count\" : %llu,"    \
        "\"station\" : %u,"           \
        "\"control_state\" : \"%s\""  \
      "}"                             \
    "}"

#define UPDATE_2_FORMAT               \
    "{"                               \
      "\"jsonrpc\": \"2.0\","         \
      "\"method\": \"update_2\","     \
      "\"params\": {"                 \
        "\"configured\" : %u,"        \
        "\"running\" : 0,"            \
        "\"config_type\" : \"%s\","   \
        "\"recording\" : %u,"         \
        "\"station\" : %u,"           \
        "\"control_state\" : \"%s\""  \
      "}"                             \
    "}"

namespace Pds {

  class RunStatus;
  class ConfigSelect;
  class StateSelect;

  class ExportStatus : public QObject, public Timer {
  Q_OBJECT

  public:
    ExportStatus(RunStatus *, ConfigSelect *, StateSelect *, const char *, const char *);
    ~ExportStatus() {}
    virtual void  expired();
    virtual Task* task();
    virtual unsigned duration() const;
    virtual unsigned repetitive() const;
    int update(void);
    int mysend(char *msg);

  public slots:
    void configured(bool vv);
    void change_state(QString ss);

  private:
    Task*           _task;
    RunStatus *     _runStatus;
    ConfigSelect *  _configSelect;
    StateSelect *   _stateSelect;
    int             _period;
    int             _sendFd;
    struct sockaddr_in _sendAddr;
    Semaphore       _sem;
    bool            _configured;
    bool            _running;
    std::string     _controlState;
    unsigned        _station;
  };

};

#endif /* Pds_ExportStatus_hh */
