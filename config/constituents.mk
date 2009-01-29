libnames := appconfig
 
libsrcs_appconfig := CfgServer.cc CfgFileDb.cc

tgtnames       := server

tgtsrcs_server := server.cc
tgtlibs_server := pdsdata/xtcdata pds/service pds/collection pds/xtc pds/utility pds/config pdsapp/appconfig
tgtslib_server := /usr/lib/rt
