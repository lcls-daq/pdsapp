
#ifndef OFFLINEAPPLIANCE_HH
#define OFFLINEAPPLIANCE_HH

#include <string>
#include <vector>
#include "pds/client/Fsm.hh"
#include "pds/offlineclient/OfflineClient.hh"
#include "LogBook/Connection.h"
#include "pds/utility/PvConfigFile.hh"

// EPICS
#include "cadef.h"

// timeout value for EPICS Channel Access calls
#define OFFLINE_EPICS_TIMEOUT 1.0

#define EPICS_CLASS_NAME  "EPICS:"

namespace Pds {

//class OfflineClient;

  class OfflineAppliance : public Fsm {
    enum { NotRecording=0xffffffff };
  public:
    OfflineAppliance(OfflineClient*, const char*, int, bool);

    // Appliance methods
    Transition* transitions(Transition* tr);
    InDatagram* events     (InDatagram* dg);
    InDatagram* occurrences(InDatagram* dg);

  private:

    typedef struct parm_channel {
      chid         value_channel;
      bool         created;
      dbr_string_t value;
    } parm_channel_t;

    static int _readConfigFile( const std::string& sFnConfig, PvConfigFile::TPvList& vsPvNameList );
    static int _splitPvList( const std::string& sPvList, PvConfigFile::TPvList& vsPv );

    static const char sPvListSeparators[];

    static int _saveRunParameter(LogBook::Connection *conn, const char *instrument,
                      const char *experiment, unsigned int run, const char *parmName,
                      const char *parmValue, const char *parmDescription);

    static int _saveRunAttribute(LogBook::Connection *conn, const char *instrument,
                      const char *experiment, unsigned int run, const char *attrName,
                      const char *attrValue, const char *attrDescription);

    int _readEpicsPv(PvConfigFile::TPvList in, std::vector<std::string> & pvValues);

    const char * _path;
    const char * _instrument_name;
    const char * _experiment_name;
    unsigned int _experiment_number;
    unsigned int _run_number;
    unsigned int _station;
    const char * _parm_list_file;
    bool         _parm_list_initialized;
    int          _parm_list_size;
    parm_channel_t  *_channels;
    PvConfigFile::TPvList _vsPvNameList;
    int          _maxParms;
    bool         _verbose;
  };

}

#endif
