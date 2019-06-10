#include <stdio.h>
#include <string>
#include <vector>
#include <fstream>
#include "pds/client/Fsm.hh"
#include "pds/client/Action.hh"
#include "pdsdata/xtc/Xtc.hh"
#include "pdsdata/xtc/TypeId.hh"

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
    _client(offlineclient),
    _run_number (0),
    _parm_list_file (parm_list_file),
    _parm_list_initialized (false),
    _parm_list_size (0),
    _maxParms (maxParms),
    _verbose (verbose),
    _gFormat (gFormat)
{
  _instrument_name = offlineclient->GetInstrumentName().c_str();
  _station = offlineclient->GetStationNumber();
  _experiment_name = offlineclient->GetExperimentName().c_str();

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
    std::map<std::string, std::string> vsPvNameValuePairs;
    string sConfigFileWarning;

    // retrieve run # that was allocated earlier
    RunInfo& rinfo = *reinterpret_cast<RunInfo*>(tr);
    _run_number = rinfo.run();
    printf("Storing BeginRun LogBook information for %s:%u/%s Run #%u\n",
            _instrument_name, _station, _experiment_name, _run_number);
    try {
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
            vsPvNameValuePairs[_vsPvNameList[iPv].sPvName] = vsPvValueList[iPv];
          }
          _client->reportParams(_run_number, vsPvNameValuePairs);
        }
    } catch (const std::runtime_error& e){
        printf("Caught exception registering parameters %s\n", e.what());
    }
  } else if ((tr->id()==TransitionId::EndRun) &&
           (_run_number != NotRecording)) {

    printf("Storing EndRun LogBook information for %s:%u/%s Run #%u\n",
            _instrument_name, _station, _experiment_name, _run_number);
    try {
      _client->EndCurrentRun();
    } catch (const std::runtime_error& e){
        printf("Caught exception ending the run %s\n", e.what());
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
