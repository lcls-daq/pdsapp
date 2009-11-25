
#ifndef OFFLINEAPPLIANCE_HH
#define OFFLINEAPPLIANCE_HH

#include <string>
#include <vector>
#include "pds/client/Fsm.hh"
#include "pds/offlineclient/OfflineClient.hh"
#include "LogBook/Connection.h"

// EPICS
#include "cadef.h"

// timeout value for EPICS Channel Access calls
#define OFFLINE_EPICS_TIMEOUT 1.0

namespace Pds {

//class OfflineClient;

  class OfflineAppliance : public Fsm {
    enum { NotRecording=0xffffffff };
  public:
    OfflineAppliance(OfflineClient*, const char*);

    // Appliance methods
    Transition* transitions(Transition* tr);
    InDatagram* events     (InDatagram* dg);
    InDatagram* occurrences(InDatagram* dg);

  private:

    typedef std::vector<std::string> TPvList;

    typedef struct parm_channel {
      chid         id;
      bool         created;
      dbr_string_t value;
    } parm_channel_t;

    static int _readConfigFile( const std::string& sFnConfig, TPvList& vsPvNameList );
    static int _splitPvList( const std::string& sPvList, TPvList& vsPv );

    static const char sPvListSeparators[];

    static int _saveRunParameter(LogBook::Connection *conn, const char *instrument,
                      const char *experiment, unsigned int run, const char *parmName,
                      const char *parmValue, const char *parmDescription);

    int _readEpicsPv(TPvList in, TPvList& out);

    const char * _path;
    const char * _instrument_name;
    const char * _experiment_name;
    unsigned int _experiment_number;
    unsigned int _run_number;
    const char * _parm_list_file;
    bool         _parm_list_initialized;
    parm_channel_t  *_channels;
  };

}

#endif
