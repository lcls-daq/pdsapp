# beamline viewing

#libnames    := blv

#libsrcs_blv := ShmOutlet.cc IdleStream.cc

#  What is this for?
CPPFLAGS += -D_ACQIRIS -D_LINUX
CPPFLAGS += -fopenmp
DEFINES += -fopenmp

ifneq ($(findstring x86_64,$(tgt_arch)),)
tgtnames    := pimblvedt
tgtnames    += evrbld pimbldedt ipmbld
tgtnames    += netfifo netfwd
endif

ifneq ($(findstring i386-linux,$(tgt_arch)),)
tgtnames    := evrblv pimblv
tgtnames    += evrbld pimbld ipmbld cambld
tgtnames    += netfifo netfwd
tgtnames    += ipmbldsim
#tgtnames    += acqbld
endif

commonlibs  := pdsdata/xtcdata pdsdata/appdata pdsdata/psddl_pdsdata pdsdata/compressdata
commonlibs  += pds/service pds/collection pds/xtc pds/mon pds/vmon pds/utility pds/management pds/client pds/config
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

cam_libs := pdsdata/psddl_pdsdata

tgtsrcs_evrblv := evrblv.cc IdleStream.cc
tgtincs_evrblv := evgr pdsdata/include ndarray/include
tgtlibs_evrblv := $(commonlibs)
tgtlibs_evrblv += evgr/evr evgr/evg 
tgtlibs_evrblv += pds/evgr pds/configdata
tgtslib_evrblv := /usr/lib/rt

tgtsrcs_pimblv := pimblv.cc ShmOutlet.cc IdleStream.cc 
tgtlibs_pimblv := $(commonlibs) $(cam_libs)
tgtlibs_pimblv += pds/camera pds/configdata
tgtlibs_pimblv += $(leutron_libs) 
tgtincs_pimblv := leutron/include pdsdata/include ndarray/include

tgtsrcs_pimblvedt := pimblvedt.cc ShmOutlet.cc IdleStream.cc 
tgtlibs_pimblvedt := $(commonlibs) $(cam_libs) pds/configdata
tgtlibs_pimblvedt += pds/camera
tgtlibs_pimblvedt += $(edt_libs)
tgtslib_pimblvedt := $(USRLIBDIR)/rt $(USRLIBDIR)/dl
tgtincs_pimblvedt := edt/include pdsdata/include ndarray/include

datalibs := pdsdata/xtcdata pdsdata/psddl_pdsdata

tgtsrcs_evrbld := evrbld.cc EvrBldManager.cc IdleStream.cc PipeApp.cc
tgtlibs_evrbld := $(commonlibs) $(datalibs)
tgtlibs_evrbld += pdsapp/configdb $(qtlibdir)
tgtlibs_evrbld += evgr/evr evgr/evg
tgtlibs_evrbld += pds/evgr pds/configdata
tgtslib_evrbld := $(qtslibdir)
tgtincs_evrbld := evgr pdsdata/include ndarray/include

tgtsrcs_acqbld := acqbld.cc ToAcqBldEventWire.cc EvrBldServer.cc PipeStream.cc
tgtincs_acqbld := acqiris pdsdata/include ndarray/include 
tgtlibs_acqbld := $(commonlibs) $(datalibs) pds/acqiris acqiris/AqDrv4
tgtlibs_acqbld += pdsapp/configdb $(qtlibdir)
tgtlibs_acqbld += pds/ipimb
tgtslib_acqbld := $(USRLIBDIR)/rt $(qtslibdir)

tgtsrcs_ipmbld := ipmbld.cc ToIpmBldEventWire.cc EvrBldServer.cc PipeStream.cc
tgtlibs_ipmbld := $(commonlibs) $(datalibs)
tgtlibs_ipmbld += pdsapp/configdb $(qtlibdir)
tgtlibs_ipmbld += pds/ipimb
tgtslib_ipmbld := $(qtslibdir)
tgtincs_ipmbld:= pdsdata/include ndarray/include 

tgtsrcs_ipmbldsim := ipmbldsim.cc ToIpmBldEventWire.cc EvrBldServer.cc PipeStream.cc
tgtlibs_ipmbldsim := $(commonlibs) $(datalibs)
tgtlibs_ipmbldsim += pdsapp/configdb $(qtlibdir)
tgtlibs_ipmbldsim += pds/ipimb
tgtslib_ipmbldsim := $(qtslibdir)
tgtincs_ipmbldsim := pdsdata/include ndarray/include 

tgtsrcs_pimbld := pimbld.cc ToPimBldEventWire.cc ToBldEventWire.cc EvrBldServer.cc PipeStream.cc
tgtlibs_pimbld := $(commonlibs) $(datalibs)
tgtlibs_pimbld += pdsapp/configdb $(qtlibdir)
tgtlibs_pimbld += pds/camera pds/configdata
tgtlibs_pimbld += $(leutron_libs)
tgtslib_pimbld := $(qtslibdir)
tgtincs_pimbld := leutron/include pdsdata/include ndarray/include

tgtsrcs_cambld := cambld.cc ToOpalBldEventWire.cc ToPimBldEventWire.cc ToBldEventWire.cc EvrBldServer.cc PipeStream.cc
tgtlibs_cambld := $(commonlibs) $(datalibs)
tgtlibs_cambld += pdsapp/configdb $(qtlibdir)
tgtlibs_cambld += pds/camera pds/clientcompress pds/configdata
tgtlibs_cambld += $(leutron_libs)
tgtslib_cambld := $(qtslibdir)
tgtincs_cambld := leutron/include pdsdata/include ndarray/include 

tgtsrcs_pimbldedt := pimbldedt.cc ToPimBldEventWire.cc ToBldEventWire.cc EvrBldServer.cc PipeStream.cc
tgtlibs_pimbldedt := $(commonlibs) $(datalibs) pds/configdata
tgtlibs_pimbldedt += pdsapp/configdb $(qtlibdir)
tgtlibs_pimbldedt += pds/camera
tgtlibs_pimbldedt += $(edt_libs)
tgtslib_pimbldedt := $(qtslibdir)
tgtincs_pimbldedt := edt/include pdsdata/include ndarray/include

tgtsrcs_netfifo := netfifo.cc
tgtlibs_netfifo := pds/service
tgtslib_netfifo := $(USRLIBDIR)/rt

tgtsrcs_netfwd := netfwd.cc
tgtlibs_netfwd := pds/service pds/utility pds/collection pds/vmon pds/mon pds/xtc pdsdata/xtcdata
tgtslib_netfwd := $(USRLIBDIR)/rt
tgtincs_netfwd := pdsdata/include
