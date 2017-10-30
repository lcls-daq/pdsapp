libnames := pycdb pydaq py3cdb py3daq

libsrcs_pycdb := pycdb.cc
libincs_pycdb := python/include/python2.5
libincs_pycdb += pdsdata/include ndarray/include boost/include  
liblibs_pycdb := pdsdata/xtcdata pdsdata/psddl_pdsdata
liblibs_pycdb += pds/service pds/utility pds/xtc pds/collection pds/vmon pds/mon
liblibs_pycdb += pds/configdata pds/config
liblibs_pycdb += pds/configdbc pds/configsql pds/confignfs offlinedb/mysqlclient
liblibs_pycdb += pdsapp/configdb
libslib_pycdb := $(USRLIBDIR)/rt

libsrcs_py3cdb := py3cdb.cc
libincs_py3cdb := python3/include/python3.6m
libincs_py3cdb += pdsdata/include ndarray/include boost/include
liblibs_py3cdb := pdsdata/xtcdata pdsdata/psddl_pdsdata
liblibs_py3cdb += pds/service pds/utility pds/xtc pds/collection pds/vmon pds/mon
liblibs_py3cdb += pds/configdata pds/config
liblibs_py3cdb += pds/configdbc pds/configsql pds/confignfs offlinedb/mysqlclient
liblibs_py3cdb += pdsapp/configdb
libslib_py3cdb := $(USRLIBDIR)/rt

CPPFLAGS += -fno-strict-aliasing

libsrcs_pydaq := pydaq.cc
libincs_pydaq := python/include/python2.5
libincs_pydaq += pdsdata/include ndarray/include boost/include  
liblibs_pydaq += pdsdata/xtcdata pdsdata/psddl_pdsdata pds/configdata
libslib_pydaq := $(USRLIBDIR)/rt

libsrcs_py3daq := py3daq.cc
libincs_py3daq := python3/include/python3.6m
libincs_py3daq += pdsdata/include ndarray/include boost/include
liblibs_py3daq += pdsdata/xtcdata pdsdata/psddl_pdsdata pds/configdata
libslib_py3daq := $(USRLIBDIR)/rt
