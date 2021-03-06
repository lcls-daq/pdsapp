# IPIMB BLD

#libnames    := bldIpimblib

libsrcs_bldIpimblib := BldIpimbStream.cc  ToBldEventWire.cc EvrBldManager.cc EvrBldServer.cc
libincs_bldIpimblib := evgr pdsdata/include ndarray/include boost/include  
tgtnames    := bldIpimb
ifneq ($(findstring x86_64,$(tgt_arch)),)
tgtnames    += bldipimbclient
endif

commonlibs  := pdsdata/xtcdata pdsdata/appdata
commonlibs  += pds/service pds/collection pds/xtc pds/mon pds/vmon pds/utility pds/management pds/client pds/config mysql/mysqlclient

datalibs := pdsdata/xtcdata pdsdata/psddl_pdsdata

#ifeq ($(shell uname -m | egrep -c '(x86_|amd)64$$'),1)
#ARCHCODE=64
#else
#ARCHCODE=32
#endif

qt_libs := $(qtlibdir)

tgtsrcs_bldIpimb := BldIpimb.cc
tgtsrcs_bldIpimb += $(libsrcs_bldIpimblib)
tgtincs_bldIpimb := evgr pdsdata/include ndarray/include boost/include  
tgtlibs_bldIpimb := $(commonlibs) $(datalibs) evgr/evr evgr/evg pds/evgr pds/ipimb pdsapp/configdb
#tgtlibs_bldIpimb := $(commonlibs) $(datalibs) evgr/evr evgr/evg pds/evgr pds/ipimb pdsapp/configdb pdsapp/bldIpimblib
tgtlibs_bldIpimb += pds/configdata pds/configdbc pds/confignfs pds/configsql
#tgtslib_bldIpimb := $(USRLIBDIR)/rt $(USRLIBDIR)/mysql/mysqlclient
tgtslib_bldIpimb := $(USRLIBDIR)/rt

tgtsrcs_bldipimbclient := bldIpimbClient.cc bldIpimbClient_moc.cc
tgtlibs_bldipimbclient := $(datalibs) pdsapp/configdb pdsapp/configdbg pds/configdata pds/configdbc pds/confignfs pds/configsql mysql/mysqlclient
tgtlibs_bldipimbclient += $(qt_libs)
#tgtslib_bldipimbclient := $(USRLIBDIR)/rt $(qtslibdir) $(USRLIBDIR)/mysql/mysqlclient
tgtslib_bldipimbclient := $(USRLIBDIR)/rt $(qtslibdir)
tgtincs_bldipimbclient := pdsdata/include $(qtincdir)
