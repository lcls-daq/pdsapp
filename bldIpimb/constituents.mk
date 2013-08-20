# IPIMB BLD

#libnames    := bldIpimblib

libsrcs_bldIpimblib := BldIpimbStream.cc  ToBldEventWire.cc EvrBldManager.cc EvrBldServer.cc
libincs_bldIpimblib := evgr
tgtnames    := bldIpimb


commonlibs  := pdsdata/xtcdata pdsdata/appdata
commonlibs  += pds/service pds/collection pds/xtc pds/mon pds/vmon pds/utility pds/management pds/client pds/config pdsdata/aliasdata

#  Get datalibs macro
include ../../pdsdata/packages.mk

#ifeq ($(shell uname -m | egrep -c '(x86_|amd)64$$'),1)
#ARCHCODE=64
#else
ARCHCODE=32
#endif

tgtsrcs_bldIpimb := BldIpimb.cc
tgtsrcs_bldIpimb += $(libsrcs_bldIpimblib)
tgtincs_bldIpimb := evgr
tgtlibs_bldIpimb := $(commonlibs) $(datalibs) evgr/evr evgr/evg pds/evgr pds/ipimb pdsapp/configdb
#tgtlibs_bldIpimb := $(commonlibs) $(datalibs) evgr/evr evgr/evg pds/evgr pds/ipimb pdsapp/configdb pdsapp/bldIpimblib
tgtlibs_bldIpimb += $(qtlibdir)
tgtslib_bldIpimb := $(USRLIBDIR)/rt $(qtslibdir)


