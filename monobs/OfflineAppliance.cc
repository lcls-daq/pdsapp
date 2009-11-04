#include <stdio.h>
#include <string>
#include <vector>
#include <fstream>
#include "pds/client/Fsm.hh"
#include "pds/client/Action.hh"
#include "pdsdata/xtc/Xtc.hh"
#include "pdsdata/xtc/TypeId.hh"

#include "LogBook/Connection.h"
#include "OfflineAppliance.hh"

#include "cadef.h"

namespace Pds
{

using std::string;

const char OfflineAppliance::sPvListSeparators[] = " ,;\t\r\n#";

//
// OfflineAppliance
//
OfflineAppliance::OfflineAppliance(OfflineClient* offlineclient, const char *parm_list_file) :
    _run_number (0),
    _parm_list_file (parm_list_file)
{
  _path = offlineclient->GetPath();
  _instrument_name = offlineclient->GetInstrumentName();
  _experiment_name = offlineclient->GetExperimentName();
}

//
// events
//
InDatagram* OfflineAppliance::events(InDatagram* in) {
  return in;
}

//
// transitions
//
Transition* OfflineAppliance::transitions(Transition* tr) {
  LogBook::Connection * conn = NULL;
  LusiTime::Time now;

  if (tr->id()==TransitionId::BeginRun) {
    int parm_list_size = 0;
    int parm_read_count = 0;
    int parm_save_count = 0;
    TPvList vsPvNameList, vsPvValueList;

    // retrieve run # that was allocated earlier
    RunInfo& rinfo = *reinterpret_cast<RunInfo*>(tr);
    _run_number = rinfo.run();
    _experiment_number = rinfo.experiment();

    printf("Storing BeginRun LogBook information for %s/%s Run #%u\n",
            _instrument_name, _experiment_name, _run_number);
    try {
      conn = LogBook::Connection::open(_path);

      if (conn != NULL) {
          // LogBook: begin transaction
          conn->beginTransaction();

          // LogBook: begin run
          now = LusiTime::Time::now();
          conn->beginRun(_instrument_name,
                         _experiment_name,
                         _run_number, "DATA", now); // DATA/CALIB

          // save metadata
          if (NULL == _parm_list_file) {
            printf("No run parameter list file specified.  Metadata will not be saved in LogBook.\n");
          } else if (_readConfigFile(_parm_list_file, vsPvNameList )) {
            printf ("Error: reading PV names from %s failed\n", _parm_list_file);
          } else {
            parm_list_size = (int) vsPvNameList.size();
            parm_read_count = _readEpicsPv(vsPvNameList, vsPvValueList);
            if (parm_read_count != parm_list_size) {
              printf("Error: read %d of %d PVs\n", parm_read_count, parm_list_size);
            }
            for ( int iPv = 0; iPv < parm_list_size; iPv++ ) {
              if (strlen(vsPvValueList[iPv].c_str()) == 0) {
                // skip empty values
                continue;
              }
              if (_saveRunParameter(conn, _instrument_name,
                            _experiment_name, _run_number,
                            vsPvNameList[iPv].c_str(), vsPvValueList[iPv].c_str(), "PV")) {
                printf("Error: storing PV %s in LogBook failed\n", vsPvNameList[iPv].c_str());
              } else {
                ++parm_save_count;
              }
            }
          }

          // LogBook: commit transaction
          conn->commitTransaction();
      } else {
          printf("LogBook::Connection::connect() failed\n");
      }

    } catch (const LogBook::ValueTypeMismatch& e) {
      printf ("Parameter type mismatch %s:\n", e.what());

    } catch (const LogBook::WrongParams& e) {
      printf ("Problem with parameters %s:\n", e.what());
    
    } catch (const LogBook::DatabaseError& e) {
      printf ("Database operation failed: %s\n", e.what());
    }
    if (NULL == _parm_list_file) {
      printf("Completed storing BeginRun LogBook information\n");
    } else {
      printf("Completed storing BeginRun LogBook information (%d of %d run parameters saved)\n",
              parm_save_count, parm_list_size);
    }

    if (conn != NULL) {
      // LogBook: close connection
      delete conn ;
    }

  } else if (tr->id()==TransitionId::EndRun) {

    printf("Storing EndRun LogBook information for %s/%s Run #%u\n",
            _instrument_name, _experiment_name, _run_number);
    try {
      conn = LogBook::Connection::open(_path);

      if (conn != NULL) {
          // begin transaction
          conn->beginTransaction();

          // end run
          now = LusiTime::Time::now();
          conn->endRun(_instrument_name, _experiment_name,
                         _run_number, now);
          // commit transaction
          conn->commitTransaction();
      } else {
          printf("Error: opening LogBook connection failed\n");
      }

    } catch (const LogBook::ValueTypeMismatch& e) {
      printf ("Parameter type mismatch %s:\n", e.what());

    } catch (const LogBook::WrongParams& e) {
      printf ("Problem with parameters %s:\n", e.what());
    
    } catch (const LogBook::DatabaseError& e) {
      printf ("Database operation failed: %s\n", e.what());
    }
    printf("Completed storing EndRun LogBook information\n");

    if (conn != NULL) {
      // close connection
      delete conn ;
    }
  }
  return tr;
}

//
// occurrences
//
InDatagram* OfflineAppliance::occurrences(InDatagram* in) {
  return in;
}


//
// private static member functions
//

int OfflineAppliance::_readConfigFile( const std::string& sFnConfig, TPvList& vsPvNameList )
{
        
    std::ifstream ifsConfig( sFnConfig.c_str() );    
    if ( !ifsConfig ) return 1; // Cannot open file
    
    while ( !ifsConfig.eof() )
    {
        string sLine;
        std::getline( ifsConfig, sLine );    
        if ( sLine[0] == '#' ) continue; // skip comment lines, whcih begin with '#'
        
        _splitPvList( sLine, vsPvNameList );
    }
        
    return 0;
}

int OfflineAppliance::_splitPvList( const string& sPvList, TPvList& vsPvList )
{       
    unsigned int uOffsetStart = sPvList.find_first_not_of( OfflineAppliance::sPvListSeparators, 0 );
    while ( uOffsetStart != string::npos )      
    {        
        unsigned uOffsetEnd = sPvList.find_first_of( OfflineAppliance::sPvListSeparators, uOffsetStart+1 );
        
        if ( uOffsetEnd == string::npos )        
        {
            vsPvList.push_back( sPvList.substr( uOffsetStart, string::npos ) );
            break;
        }
        
        vsPvList.push_back( sPvList.substr( uOffsetStart, uOffsetEnd - uOffsetStart ) );
        if ( sPvList[uOffsetEnd] == '#' ) break; // skip the remaining characters
        
        uOffsetStart = sPvList.find_first_not_of( sPvListSeparators, uOffsetEnd+1 );        
    }
    return 0;
}

//
// _saveRunParameter -
//
// RETURNS: 0 if successful, otherwise 1.
//
int OfflineAppliance::_saveRunParameter(LogBook::Connection *conn, const char *instrument,
                      const char *experiment, unsigned int run, const char *parmName,
                      const char *parmValue, const char *parmDescription)
{
    LogBook::ParamInfo paramBuf;
    bool dbError = false;
    bool parmError = false;
    bool parmFound = false;

    if (!conn || !instrument || !experiment || !parmName || !parmValue || !parmDescription) {
      return 1;   // invalid parameter
    }

    try {
        parmFound = conn->getParamInfo(paramBuf, instrument, experiment, parmName);
    } catch (const LogBook::WrongParams& e) {
      parmError = true;
      printf ("_saveRunParameter: Problem with parameters %s:\n", e.what());
    
    } catch (const LogBook::DatabaseError& e) {
      dbError = true;
      printf ("_saveRunParameter(): Database operation failed: %s\n", e.what());
    } 

    if (!parmError && !dbError && !parmFound) {
      // create run parameter
      parmError = false;
      try {
        conn->createRunParam(instrument, experiment, parmName, "TEXT",
                             parmDescription);
      } catch (const LogBook::WrongParams& e) {
        parmError = true;
        printf ("createRunParam(): Problem with parameters %s:\n", e.what());

      } catch (const LogBook::DatabaseError& e) {
        dbError = true;
        printf ("createRunParam(): Database operation failed: %s\n", e.what());
      }
    }

    if (!dbError && !parmError) {
      try {
        conn->setRunParam(instrument, experiment, run, parmName,
                               parmValue, "run control", true);
      } catch (const LogBook::ValueTypeMismatch& e) {
        parmError = true;
        printf ("setRunParam(): Parameter type mismatch %s:\n", e.what());

      } catch (const LogBook::WrongParams& e) {
        parmError = true;
        printf ("setRunParam(): Problem with parameters %s:\n", e.what());

      } catch (const LogBook::DatabaseError& e) {
        dbError = true;
        printf ("setRunParam(): Database operation failed: %s\n", e.what());
      }
  }

  return (dbError || parmError) ? 1 : 0;
}

//
// _readEpicsPv -
//
// RETURNS: The number of PVs successfully read.
//
int OfflineAppliance::_readEpicsPv(TPvList in, TPvList& out)
{
  chid chan[in.size()];
  char chan_status[in.size()];
  dbr_string_t epics_string[in.size()];
  int  status;
  int read_count = 0;
  int ix;
  bool epics_initialized = false;

  // Initialize Channel Access
  status = ca_task_initialize();
  if (ECA_NORMAL == status) {
    epics_initialized = true;
  }
  else {
    SEVCHK(status, NULL);
    printf("Error: %s: Unable to initialize Channel Access", __FUNCTION__);
    // On failure, return array of empty strings
    for (ix = 0; ix < (int)in.size(); ix ++) {
      out.push_back("");
    }
  }

  if (epics_initialized) {
    // Create channels
    for (ix = 0; ix < (int)in.size(); ix ++) {
      status = ca_create_channel(in[ix].c_str(), 0, 0, 0, &chan[ix]);
      if (ECA_NORMAL == status) {
        chan_status[ix] = 1;
      }
      else {
        SEVCHK(status, NULL);
        chan_status[ix] = 0;
        printf("Error: %s: problem establishing connection to %s.", __FUNCTION__, in[ix].c_str());
      }
    }

    // Send the requests and wait for channels to be found.
    status = ca_pend_io(OFFLINE_EPICS_TIMEOUT);
    SEVCHK(status, NULL);

    // Make get requests
    for (ix = 0; ix < (int)in.size(); ix ++) {
      if ((chan_status[ix]) && (cs_conn == ca_state(chan[ix]))) {
        // channel is connected
        status = ca_bget(chan[ix], epics_string[ix]);
        if (ECA_NORMAL == status){
          ++read_count;   // increment count of PVs successfully read
        } else {
          SEVCHK(status, NULL);
          printf("Error in call to ca_bget()");
          epics_string[ix][0] = '\0';   // empty string is default
        }
      } else {
        // channel is not connected
        printf("Error: failed to connect PV %s\n", in[ix].c_str());
        epics_string[ix][0] = '\0';   // empty string is default
      }
    }

    // Flush I/O
    status = ca_flush_io();
    SEVCHK(status, NULL);

    // Give operations time to complete
    status = ca_pend_io(2 * OFFLINE_EPICS_TIMEOUT);
    SEVCHK(status, NULL);

    if (ECA_TIMEOUT == status) {
      printf("Error: %s: Get Timed Out\n", __FUNCTION__);
      // From ca_pend_io() manual:
      //   "If ECA_TIMEOUT is returned then failure must be assumed for all
      //   outstanding queries."
      read_count = 0;
    }
    for (ix = 0; ix < (int)in.size(); ix ++) {
      out.push_back((read_count > 0) ? epics_string[ix]: "");
      // shut down and reclaim resources associated with channel
      status = ca_clear_channel(chan[ix]);
      SEVCHK(status, NULL);
    }

    // free channel access resources
    ca_context_destroy();
  }
  return read_count;
}

} // namespace Pds
