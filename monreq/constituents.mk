#
#  Build a "monreqserver" executable
tgtnames := monreqserver

tgtsrcs_monreqserver := monreqserver.cc
tgtlibs_monreqserver := pdsdata/appdata pds/offlineclient offlinedb/mysqlclient offlinedb/offlinedb
tgtlibs_monreqserver += pds/service pds/collection pds/xtc pds/mon pds/vmon pds/utility pds/management pds/client pds/configdata pdsdata/xtcdata pdsdata/psddl_pdsdata
tgtlibs_monreqserver += pds/mon pdsdata/indexdata pdsdata/smalldata pdsdata/psddl_pdsdata pds/monreq pdsapp/tools
tgtslib_monreqserver := $(USRLIBDIR)/rt
tgtincs_monreqserver := pdsdata/include

tgtnames :=