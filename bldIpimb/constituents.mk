# IPIMB BLD

libnames    := bldIpimblib

libsrcs_bldIpimblib := BldIpimbStream.cc  ToBldEventWire.cc EvrBldManager.cc EvrBldServer.cc
tgtnames    := bldIpimb


commonlibs  := pdsdata/xtcdata pdsdata/appdata
commonlibs  += pds/service pds/collection pds/xtc pds/mon pds/vmon pds/utility pds/management pds/client pds/config 

#ifeq ($(shell uname -m | egrep -c '(x86_|amd)64$$'),1)
#ARCHCODE=64
#else
ARCHCODE=32
#endif

tgtsrcs_bldIpimb := BldIpimb.cc
tgtincs_bldIpimb := evgr
tgtlibs_bldIpimb := $(commonlibs)  evgr/evr evgr/evg pds/evgr  pdsdata/evrdata  pdsdata/ipimbdata pds/ipimb pdsapp/bldIpimblib
tgtslib_bldIpimb := /usr/lib/rt


