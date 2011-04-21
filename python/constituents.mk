libnames := pycdb

libsrcs_pycdb := pycdb.cc
libincs_pycdb := python/include/python2.5
liblibs_pycdb := pdsdata/xtcdata pdsdata/acqdata
liblibs_pycdb += pdsdata/camdata pdsdata/opal1kdata
liblibs_pycdb += pdsdata/pulnixdata pdsdata/princetondata
liblibs_pycdb += pdsdata/pnccddata pdsdata/ipimbdata
liblibs_pycdb += pdsdata/evrdata pdsdata/encoderdata
liblibs_pycdb += pdsdata/controldata pdsdata/epics 
liblibs_pycdb += pdsdata/cspaddata pdsdata/lusidata
liblibs_pycdb += pdsapp/configdb
libslib_pycdb := $(USRLIBDIR)/rt
