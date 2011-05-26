# constituents for the segment level device production code ONLY

CPPFLAGS += -D_ACQIRIS -D_LINUX

tgtnames    := 	evr \
		evrstandalone \
		evrsnoop \
		acq \
		opal1k \
		epicsArch \
		rceProxy \
		encoder \
		bld \
		princeton \
		ipimb \
		lusidiag \
		tm6740 \
		pimimage \
		fccd     \
		cspad \
		simcam

commonlibs  := pdsdata/xtcdata pdsdata/appdata
commonlibs  += pds/service pds/collection pds/xtc pds/mon pds/vmon pds/utility pds/management pds/client pds/config 

#tgtsrcs_xamps := xamps.cc
#tgtincs_xamps := xamps
#tgtlibs_xamps := $(commonlibs) pdsdata/cspaddata pds/xamps pds/pgp
#tgtslib_xamps := /usr/lib/rt
#CPPFLAGS += -fno-strict-aliasing
#CPPFLAGS += -fopenmp
#DEFINES += -fopenmp

tgtsrcs_cspad := cspad.cc
tgtincs_cspad := cspad
tgtlibs_cspad := $(commonlibs) pdsdata/cspaddata pds/cspad pds/pgp
tgtslib_cspad := /usr/lib/rt
CPPFLAGS += -fno-strict-aliasing
#CPPFLAGS += -fopenmp
#DEFINES += -fopenmp

tgtsrcs_acq := acq.cc
tgtincs_acq := acqiris
tgtlibs_acq := $(commonlibs) pdsdata/acqdata pds/acqiris acqiris/AqDrv4
tgtslib_acq := /usr/lib/rt

tgtsrcs_ipimb := ipimb.cc
tgtincs_ipimb := ipimb
tgtlibs_ipimb := $(commonlibs) pdsdata/ipimbdata pds/ipimb 
tgtslib_ipimb := /usr/lib/rt

tgtsrcs_lusidiag := lusidiag.cc
tgtincs_lusidiag := lusidiag
tgtlibs_lusidiag := $(commonlibs) pdsdata/ipimbdata pdsdata/lusidata pds/ipimb
tgtslib_lusidiag := /usr/lib/rt

tgtsrcs_encoder := encoder.cc
tgtincs_encoder := encoder
tgtlibs_encoder := $(commonlibs) pdsdata/encoderdata pds/encoder
tgtslib_encoder := /usr/lib/rt

tgtsrcs_evr := evr.cc
tgtincs_evr := evgr
tgtlibs_evr := pdsdata/xtcdata pdsdata/evrdata
tgtlibs_evr += evgr/evr evgr/evg 
tgtlibs_evr += $(commonlibs) pds/evgr 
tgtslib_evr := /usr/lib/rt

tgtsrcs_evrstandalone := evrstandalone.cc
tgtincs_evrstandalone := evgr
tgtlibs_evrstandalone := $(commonlibs) pdsdata/evrdata
tgtlibs_evrstandalone += evgr/evr evgr/evg 
tgtlibs_evrstandalone += pds/evgr
tgtslib_evrstandalone := /usr/lib/rt

tgtsrcs_evrsnoop := evrsnoop.cc
tgtincs_evrsnoop := evgr
tgtlibs_evrsnoop := $(commonlibs) pdsdata/evrdata
tgtlibs_evrsnoop += evgr/evr evgr/evg 
tgtlibs_evrsnoop += pds/evgr
tgtslib_evrsnoop := /usr/lib/rt

#ifeq ($(shell uname -m | egrep -c '(x86_|amd)64$$'),1)
#ARCHCODE=64
#else
ARCHCODE=32
#endif

leutron_libs := leutron/lvsds.34.${ARCHCODE}
leutron_libs += leutron/LvCamDat.34.${ARCHCODE}
leutron_libs += leutron/LvSerialCommunication.34.${ARCHCODE}

tgtsrcs_opal1k := opal1k.cc 
tgtlibs_opal1k := $(commonlibs) pdsdata/opal1kdata pdsdata/fccddata pdsdata/pulnixdata pdsdata/camdata
tgtlibs_opal1k += pds/camera
tgtlibs_opal1k += $(leutron_libs)
tgtincs_opal1k := leutron/include

tgtsrcs_tm6740 := tm6740.cc 
tgtlibs_tm6740 := $(commonlibs) pdsdata/opal1kdata pdsdata/fccddata pdsdata/pulnixdata pdsdata/camdata
tgtlibs_tm6740 += pds/camera
tgtlibs_tm6740 += $(leutron_libs)
tgtincs_tm6740 := leutron/include

tgtsrcs_pimimage := pimimage.cc 
tgtlibs_pimimage := $(commonlibs) pdsdata/opal1kdata pdsdata/fccddata pdsdata/pulnixdata pdsdata/camdata
tgtlibs_pimimage += pds/camera
tgtlibs_pimimage += $(leutron_libs)
tgtincs_pimimage := leutron/include

tgtsrcs_fccd := fccd.cc
tgtlibs_fccd := $(commonlibs) pdsdata/opal1kdata pdsdata/fccddata pdsdata/pulnixdata pdsdata/camdata
tgtlibs_fccd += pds/camera
tgtlibs_fccd += $(leutron_libs)
tgtincs_fccd := leutron/include

tgtsrcs_epicsArch := epicsArch.cc
tgtlibs_epicsArch := $(commonlibs) pdsdata/epics pds/epicsArch epics/ca epics/Com
tgtslib_epicsArch := /usr/lib/rt

tgtsrcs_bld := bld.cc 
tgtlibs_bld := $(commonlibs) pdsdata/evrdata
tgtslib_bld := /usr/lib/rt

tgtsrcs_rceProxy := rceProxy.cc
tgtlibs_rceProxy := $(commonlibs) pdsdata/pnccddata pds/rceProxy
tgtslib_rceProxy := /usr/lib/rt

tgtsrcs_princeton := princeton.cc
#tgtlibs_princeton := $(commonlibs) pdsdata/pnccddata pdsdata/evrdata pdsdata/princetondata pds/princeton pvcam/pvcamtest
tgtlibs_princeton := $(commonlibs) pdsdata/pnccddata pdsdata/evrdata pdsdata/princetondata pds/princeton pvcam/pvcam
tgtslib_princeton := /usr/lib/rt dl pthread

tgtsrcs_simcam := simcam.cc 
tgtlibs_simcam := $(commonlibs) pdsdata/pulnixdata pdsdata/camdata
tgtslib_simcam := pthread rt
