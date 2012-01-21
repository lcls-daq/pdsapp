# IPIMB BLD

#libnames    := bldIpimblib

libsrcs_bldIpimblib := BldIpimbStream.cc  ToBldEventWire.cc EvrBldManager.cc EvrBldServer.cc
libincs_bldIpimblib := evgr
tgtnames    := bldIpimb


commonlibs  := pdsdata/xtcdata pdsdata/appdata
commonlibs  += pds/service pds/collection pds/xtc pds/mon pds/vmon pds/utility pds/management pds/client pds/config 
datalibs := pdsdata/xtcdata pdsdata/opal1kdata pdsdata/pulnixdata pdsdata/camdata pdsdata/pnccddata pdsdata/evrdata pdsdata/acqdata pdsdata/controldata pdsdata/princetondata pdsdata/ipimbdata pdsdata/encoderdata pdsdata/fccddata pdsdata/lusidata pdsdata/cspaddata pdsdata/xampsdata pdsdata/fexampdata pdsdata/gsc16aidata
datalibs += pdsdata/timepixdata
datalibs += pdsdata/phasicsdata
datalibs += pdsdata/cspad2x2data

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
tgtlibs_bldIpimb += qt/QtGui qt/QtCore
tgtslib_bldIpimb := /usr/lib/rt


