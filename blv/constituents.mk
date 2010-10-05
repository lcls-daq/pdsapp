# beamline viewing

libnames    := blv

libsrcs_blv := ShmOutlet.cc IdleStream.cc

tgtnames    := evrblv pimblv

commonlibs  := pdsdata/xtcdata pdsdata/appdata
commonlibs  += pds/service pds/collection pds/xtc pds/mon pds/vmon pds/utility pds/management pds/client pds/config 
commonlibs  += pdsapp/blv

#ifeq ($(shell uname -m | egrep -c '(x86_|amd)64$$'),1)
#ARCHCODE=64
#else
ARCHCODE=32
#endif

leutron_libs := leutron/lvsds.34.${ARCHCODE}
leutron_libs += leutron/LvCamDat.34.${ARCHCODE}
leutron_libs += leutron/LvSerialCommunication.34.${ARCHCODE}

tgtsrcs_evrblv := evrblv.cc
tgtincs_evrblv := evgr
tgtlibs_evrblv := $(commonlibs) pdsdata/evrdata
tgtlibs_evrblv += evgr/evr evgr/evg 
tgtlibs_evrblv += pds/evgr
tgtslib_evrblv := /usr/lib/rt

tgtsrcs_pimblv := pimblv.cc 
tgtlibs_pimblv := $(commonlibs) pdsdata/opal1kdata pdsdata/fccddata pdsdata/pulnixdata pdsdata/camdata
tgtlibs_pimblv += pds/camera
tgtlibs_pimblv += $(leutron_libs)
tgtincs_pimblv := leutron/include

