
#ifndef OFFLINEAPPLIANCE_HH
#define OFFLINEAPPLIANCE_HH

#include <string>
#include <vector>
#include "pds/client/Fsm.hh"
#include "pds/offlineclient/OfflineClient.hh"
#include "LogBook/Connection.h"

// timeout value for EPICS Channel Access calls
#define OFFLINE_EPICS_TIMEOUT 1.0

namespace Pds {

//class OfflineClient;

  class OfflineAppliance : public Fsm {
  public:
    OfflineAppliance(OfflineClient*, const char*);

    // Appliance methods
    Transition* transitions(Transition* tr);
    InDatagram* events     (InDatagram* dg);
    InDatagram* occurrences(InDatagram* dg);

  private:

    typedef std::vector<std::string> TPvList;

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
  };

}

#endif
