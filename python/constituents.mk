libnames := pycdb pydaq

libsrcs_pycdb := pycdb.cc
libincs_pycdb := python/include/python2.5 pdsdata/include ndarray/include boost/include  
liblibs_pycdb := pdsdata/xtcdata pdsdata/psddl_pdsdata
liblibs_pycdb += pdsapp/configdb
liblibs_pycdb += pds/configdata
libslib_pycdb := $(USRLIBDIR)/rt 
CPPFLAGS += -fno-strict-aliasing

libsrcs_pydaq := pydaq.cc
libincs_pydaq := python/include/python2.5 pdsdata/include ndarray/include boost/include  
liblibs_pydaq += pdsdata/xtcdata pdsdata/psddl_pdsdata pds/configdata
libslib_pydaq := $(USRLIBDIR)/rt
