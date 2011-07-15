# beamline viewing

#libnames    := blv

#libsrcs_blv := ShmOutlet.cc IdleStream.cc

tgtnames    := evrblv pimblv pimbld

commonlibs  := pdsdata/xtcdata pdsdata/appdata
commonlibs  += pds/service pds/collection pds/xtc pds/mon pds/vmon pds/utility pds/management pds/client pds/config 
#commonlibs  += pdsapp/blv

#ifeq ($(shell uname -m | egrep -c '(x86_|amd)64$$'),1)
#ARCHCODE=64
#else
ARCHCODE=32
#endif

leutron_libs := leutron/lvsds.34.${ARCHCODE}
leutron_libs += leutron/LvCamDat.34.${ARCHCODE}
leutron_libs += leutron/LvSerialCommunication.34.${ARCHCODE}

tgtsrcs_evrblv := evrblv.cc IdleStream.cc
tgtincs_evrblv := evgr
tgtlibs_evrblv := $(commonlibs) pdsdata/evrdata
tgtlibs_evrblv += evgr/evr evgr/evg 
tgtlibs_evrblv += pds/evgr
tgtslib_evrblv := /usr/lib/rt

tgtsrcs_pimblv := pimblv.cc ShmOutlet.cc IdleStream.cc 
tgtlibs_pimblv := $(commonlibs) pdsdata/opal1kdata pdsdata/fccddata pdsdata/pulnixdata pdsdata/camdata
tgtlibs_pimblv += pds/camera
tgtlibs_pimblv += $(leutron_libs)
tgtincs_pimblv := leutron/include

tgtsrcs_pimbld := pimbld.cc ToBldEventWire.cc EvrBldManager.cc EvrBldServer.cc IdleStream.cc
tgtlibs_pimbld := $(commonlibs) pdsdata/opal1kdata pdsdata/fccddata pdsdata/pulnixdata pdsdata/camdata pdsdata/evrdata
tgtlibs_pimbld += pdsapp/configdb qt/QtGui qt/QtCore
tgtlibs_pimbld += pdsdata/xampsdata pdsdata/cspaddata pdsdata/lusidata
tgtlibs_pimbld += pdsdata/encoderdata pdsdata/ipimbdata pdsdata/princetondata pdsdata/controldata
tgtlibs_pimbld += pdsdata/acqdata pdsdata/pnccddata
tgtlibs_pimbld += evgr/evr evgr/evg
tgtlibs_pimbld += pds/evgr
tgtlibs_pimbld += pds/camera
tgtlibs_pimbld += $(leutron_libs)
tgtincs_pimbld := evgr
tgtincs_pimbld += leutron/include
