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

#define ca_dget(chan, pValue) \
ca_array_get(DBR_DOUBLE, 1u, chan, (dbr_double_t *)(pValue))

namespace Pds
{

using std::string;

// const char OfflineAppliance::sPvListSeparators[] = " ,;\t\r\n#";

//
// OfflineAppliance
//
OfflineAppliance::OfflineAppliance(OfflineClient* offlineclient, const char *parm_list_file, int maxParms, bool verbose, bool gFormat) :
    _run_number (0),
    _parm_list_file (parm_list_file),
    _parm_list_initialized (false),
    _parm_list_size (0),
    _maxParms (maxParms),
    _verbose (verbose),
    _gFormat (gFormat)
{
  _path = offlineclient->GetPath();
  _instrument_name = offlineclient->GetInstrumentName();
  _station = offlineclient->GetStationNumber();
  _experiment_name = offlineclient->GetExperimentName();

  string sConfigFileWarning;

  // read list of PVs one time only
  if (NULL == _parm_list_file) {
    printf("No run parameter list file specified.  Metadata will not be saved in LogBook.\n");
  }
  else {
    printf("%s: Reading list of PVs from '%s'\n", __FUNCTION__, _parm_list_file);

    PvConfigFile configFile(_parm_list_file, (float)1.0, 10, _maxParms, _verbose);
    if (configFile.read(_vsPvNameList, sConfigFileWarning)) {
      printf ("Error: reading PV names from '%s' failed\n", _parm_list_file);
      printf ("%s\n", sConfigFileWarning.c_str());
    } else {
      _parm_list_size = _vsPvNameList.size();
      if (_parm_list_size > 0) {
        _channels = new parm_channel_t[_parm_list_size];
        printf("Completed reading list of %d PVs\n", _parm_list_size);
        for (int iPv = 0; iPv < _parm_list_size; iPv++) {
          printf("  [%d] %-30s PV: %-30s \n", iPv,
                 _vsPvNameList[iPv].sPvDescription.c_str(), _vsPvNameList[iPv].sPvName.c_str());
        }
        printf("\n");
      } else {
        printf("PV list is empty\n");
      }
    }
  }
  _parm_list_initialized = true;
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
  int errs;

  if ((tr->id()==TransitionId::BeginRun) &&
      (tr->size() == sizeof(Transition))) {
    // no RunInfo
    _run_number = NotRecording;
    return tr;
  }

  if (tr->id()==TransitionId::BeginRun) {
    int parm_read_count = 0;
    int parm_save_count = 0;
    std::vector<string> vsPvValueList;
    string sConfigFileWarning;

    // retrieve run # that was allocated earlier
    RunInfo& rinfo = *reinterpret_cast<RunInfo*>(tr);
    _run_number = rinfo.run();
    _experiment_number = rinfo.experiment();
    printf("Storing BeginRun LogBook information for %s:%u/%s Run #%u\n",
            _instrument_name, _station, _experiment_name, _run_number);
    try {
      conn = LogBook::Connection::open(_path);

      if (conn != NULL) {
        // LogBook: begin transaction
        conn->beginTransaction();

        if (_parm_list_size > 0) {
          // save metadata
          parm_read_count = _readEpicsPv(_vsPvNameList, vsPvValueList);
          if (parm_read_count != _parm_list_size) {
            printf("Error: read %d of %d PVs\n", parm_read_count, _parm_list_size);
          }
          for ( int iPv = 0; iPv < _parm_list_size; iPv++ ) {
            if (vsPvValueList[iPv].empty()) {
              // skip empty values
              continue;
            }

            errs = 0;
            if (_saveRunAttribute(conn, _instrument_name,
                          _experiment_name, _run_number,
                          _vsPvNameList[iPv].sPvName.c_str(), vsPvValueList[iPv].c_str(),
                          _vsPvNameList[iPv].sPvDescription.c_str())) {
              ++ errs;
            }
            if (_saveRunParameter(conn, _instrument_name,
                          _experiment_name, _run_number,
                          _vsPvNameList[iPv].sPvName.c_str(), vsPvValueList[iPv].c_str(),
                          _vsPvNameList[iPv].sPvDescription.c_str())) {
              ++ errs;
            }
            if (errs) {
              printf("Error: storing PV %s in LogBook failed\n", _vsPvNameList[iPv].sPvName.c_str());
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
      printf ("Parameter type mismatch: %s\n", e.what());

    } catch (const LogBook::WrongParams& e) {
      printf ("Problem with parameters: %s\n", e.what());
    
    } catch (const LogBook::DatabaseError& e) {
      printf ("Database operation failed: %s\n", e.what());
    }
    if (NULL == _parm_list_file) {
      printf("Completed storing BeginRun LogBook information\n");
    } else {
      printf("Completed storing BeginRun LogBook information (%d of %d run parameters saved)\n",
              parm_save_count, _parm_list_size);
    }

    if (conn != NULL) {
      // LogBook: close connection
      delete conn ;
    }

  }
  else if ((tr->id()==TransitionId::EndRun) &&
           (_run_number != NotRecording)) {

    printf("Storing EndRun LogBook information for %s:%u/%s Run #%u\n",
            _instrument_name, _station, _experiment_name, _run_number);
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
      printf ("Parameter type mismatch: %s\n", e.what());

    } catch (const LogBook::WrongParams& e) {
      printf ("Problem with parameters: %s\n", e.what());
    
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
      printf ("getParamInfo(): Problem with parameters: %s\n", e.what());
    
    } catch (const LogBook::DatabaseError& e) {
      dbError = true;
      printf ("getParamInfo(): Database operation failed: %s\n", e.what());
    } 

    if (!parmError && !dbError && !parmFound) {
      // create run parameter
      try {
        conn->createRunParam(instrument, experiment, parmName, "TEXT",
                             parmDescription);
      } catch (const LogBook::WrongParams& e) {
        parmError = true;
        printf ("createRunParam(): Problem with parameters: %s\n", e.what());

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
        printf ("setRunParam(): Parameter type mismatch: %s\n", e.what());

      } catch (const LogBook::WrongParams& e) {
        parmError = true;
        printf ("setRunParam(): Problem with parameters: %s\n", e.what());

      } catch (const LogBook::DatabaseError& e) {
        dbError = true;
        printf ("setRunParam(): Database operation failed: %s\n", e.what());
      }
  }

  return (dbError || parmError) ? 1 : 0;
}

//
// _saveRunAttribute -
//
// RETURNS: 0 if successful, otherwise 1.
//
int OfflineAppliance::_saveRunAttribute(LogBook::Connection *conn, const char *instrument,
                      const char *experiment, unsigned int run, const char *attrName,
                      const char *attrValue, const char *attrDescription)
{
    LogBook::AttrInfo attrBuf;
    bool dbError = false;
    bool parmError = false;
    bool parmFound = false;

    if (!conn || !instrument || !experiment || !attrName || !attrValue || !attrDescription) {
      return 1;   // invalid parameter
    }

    try {
      parmFound = conn->getAttrInfo(attrBuf, instrument, experiment, run, EPICS_CLASS_NAME, attrName);
    } catch (const LogBook::WrongParams& e) {
      parmError = true;
      printf ("getAttrInfo(): Problem with parameters: %s\n", e.what());
    
    } catch (const LogBook::DatabaseError& e) {
      dbError = true;
      printf ("getAttrInfo(): Database operation failed: %s\n", e.what());
    } 

    if (!parmError && !dbError && !parmFound) {
      // create run attribute
      try {
        conn->createRunAttr(instrument, experiment, run, EPICS_CLASS_NAME, attrName,
                             attrDescription, attrValue);
      } catch (const LogBook::WrongParams& e) {
        parmError = true;
        printf ("createRunAttr(): Problem with parameters: %s\n", e.what());

      } catch (const LogBook::DatabaseError& e) {
        dbError = true;
        printf ("createRunAttr(): Database operation failed: %s\n", e.what());
      }
    }

  return (dbError || parmError) ? 1 : 0;
}

//
// _readEpicsPv -
//
// RETURNS: The number of PVs successfully read.
//
int OfflineAppliance::_readEpicsPv(PvConfigFile::TPvList in, std::vector < std::string> & pvValues)
{
  int status;
  int read_count = 0;
  int ix;

  if (ca_current_context() == NULL) {
    // Initialize Channel Access
    status = ca_context_create(ca_disable_preemptive_callback);
    if (ECA_NORMAL == status) {
      // Create channels
      for (ix = 0; ix < (int)in.size(); ix ++) {
        status = ca_create_channel(in[ix].sPvName.c_str(), 0, 0, 0, &_channels[ix].value_channel);
        if (ECA_NORMAL == status) {
          _channels[ix].created = true;
        } else {
          SEVCHK(status, NULL);
          _channels[ix].created = false;
          printf("Error: %s: problem establishing connection to %s.", __FUNCTION__, in[ix].sPvName.c_str());
        }
      }

      // Send the requests and wait for channels to be found.
      (void) ca_pend_io(OFFLINE_EPICS_TIMEOUT);
    }
    else {
      // ca_context_create() error
      SEVCHK(status, NULL);
      printf("Error: %s: Failed to initialize Channel Access", __FUNCTION__);
    }
  }

  if (ca_current_context() != NULL) {
    chtype ttt;
    // Make get requests
    for (ix = 0; ix < (int)in.size(); ix ++) {
      if ((_channels[ix].created) && (cs_conn == ca_state(_channels[ix].value_channel))) {
        // channel is connected
        ttt = ca_field_type(_channels[ix].value_channel);
        _channels[ix].isDouble = _gFormat && ((ttt == DBF_FLOAT) || (ttt == DBF_DOUBLE));
        if (_channels[ix].isDouble) {
          status = ca_dget(_channels[ix].value_channel, &_channels[ix].dValue);
          if (ECA_NORMAL == status) {
            ++read_count;   // increment count of PVs successfully read
          } else {
            SEVCHK(status, NULL);
            printf("Error in call to ca_dget()\n");
            _channels[ix].dValue = 0.0;     // zero is default
          }
        } else {
          status = ca_bget(_channels[ix].value_channel, _channels[ix].value);
          if (ECA_NORMAL == status){
            ++read_count;   // increment count of PVs successfully read
          } else {
            SEVCHK(status, NULL);
            printf("Error in call to ca_bget()\n");
            _channels[ix].value[0] = '\0';  // empty string is default
          }
        }
      } else {
        // channel is not connected
        printf("Error: failed to connect PV %s\n", in[ix].sPvName.c_str());
        _channels[ix].isDouble = false;
        _channels[ix].value[0] = '\0';    // empty string is default
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
      if (read_count > 0) {
        if (_channels[ix].isDouble) {
          sprintf(_channels[ix].value, "%g", _channels[ix].dValue);
        }
        pvValues.push_back(_channels[ix].value);
      } else {
        pvValues.push_back("");
      }
    }
  }
  else {
    // EPICS not initialized
    // ...return array of empty strings
    for (ix = 0; ix < (int)in.size(); ix ++) {
      pvValues.push_back("");
    }
  }
  return read_count;
}

} // namespace Pds
