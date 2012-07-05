# beamline viewing

#libnames    := blv

#libsrcs_blv := ShmOutlet.cc IdleStream.cc

ifneq ($(findstring x86_64-linux,$(tgt_arch)),)
tgtnames    := pimblvedt
endif

ifneq ($(findstring i386-linux,$(tgt_arch)),)
tgtnames    := evrblv pimblv
tgtnames    += evrbld pimbld
tgtnames    += netfifo netfwd
endif

commonlibs  := pdsdata/xtcdata pdsdata/appdata
commonlibs  += pds/service pds/collection pds/xtc pds/mon pds/vmon pds/utility pds/management pds/client pds/config 
commonlibs  += pdsdata/cspad2x2data
#commonlibs  += pdsapp/blv

#ifeq ($(shell uname -m | egrep -c '(x86_|amd)64$$'),1)
#ARCHCODE=64
#else
ARCHCODE=32
#endif

leutron_libs := pds/camleutron
leutron_libs += leutron/lvsds.34.${ARCHCODE}
leutron_libs += leutron/LvCamDat.34.${ARCHCODE}
leutron_libs += leutron/LvSerialCommunication.34.${ARCHCODE}

edt_libs := pds/camedt edt/pdv

cam_libs := pdsdata/opal1kdata pdsdata/quartzdata pdsdata/fccddata pdsdata/pulnixdata pdsdata/camdata

tgtsrcs_evrblv := evrblv.cc IdleStream.cc
tgtincs_evrblv := evgr
tgtlibs_evrblv := $(commonlibs) pdsdata/evrdata
tgtlibs_evrblv += evgr/evr evgr/evg 
tgtlibs_evrblv += pds/evgr
tgtslib_evrblv := /usr/lib/rt

tgtsrcs_pimblv := pimblv.cc ShmOutlet.cc IdleStream.cc 
tgtlibs_pimblv := $(commonlibs) $(cam_libs)
tgtlibs_pimblv += pds/camera
tgtlibs_pimblv += $(leutron_libs)
tgtincs_pimblv := leutron/include

tgtsrcs_pimblvedt := pimblvedt.cc ShmOutlet.cc IdleStream.cc 
tgtlibs_pimblvedt := $(commonlibs) $(cam_libs)
tgtlibs_pimblvedt += pds/camera
tgtlibs_pimblvedt += $(edt_libs)
tgtslib_pimblvedt := $(USRLIBDIR)/rt $(USRLIBDIR)/dl
tgtincs_pimblvedt := edt/include

datalibs := pdsdata/xtcdata pdsdata/opal1kdata pdsdata/quartzdata pdsdata/pulnixdata pdsdata/camdata pdsdata/pnccddata pdsdata/evrdata pdsdata/acqdata pdsdata/controldata pdsdata/princetondata pdsdata/ipimbdata pdsdata/encoderdata pdsdata/fccddata pdsdata/lusidata pdsdata/cspaddata pdsdata/xampsdata pdsdata/fexampdata pdsdata/gsc16aidata pdsdata/timepixdata pdsdata/phasicsdata pdsdata/cspad2x2data pdsdata/oceanopticsdata pdsdata/flidata

tgtsrcs_evrbld := evrbld.cc EvrBldManager.cc IdleStream.cc PipeApp.cc
tgtlibs_evrbld := $(commonlibs) $(datalibs)
tgtlibs_evrbld += pdsapp/configdb qt/QtGui qt/QtCore
tgtlibs_evrbld += evgr/evr evgr/evg
tgtlibs_evrbld += pds/evgr
tgtincs_evrbld := evgr

tgtsrcs_pimbld := pimbld.cc ToBldEventWire.cc EvrBldServer.cc PipeStream.cc
tgtlibs_pimbld := $(commonlibs) $(datalibs)
tgtlibs_pimbld += pdsapp/configdb qt/QtGui qt/QtCore
tgtlibs_pimbld += pds/camera
tgtlibs_pimbld += $(leutron_libs)
tgtincs_pimbld := leutron/include

tgtsrcs_netfifo := netfifo.cc
tgtlibs_netfifo := pds/service
tgtslib_netfifo := $(USRLIBDIR)/rt

tgtsrcs_netfwd := netfwd.cc
tgtlibs_netfwd := pds/service pds/utility pds/collection pds/vmon pds/mon pds/xtc pdsdata/xtcdata
tgtslib_netfwd := $(USRLIBDIR)/rt
