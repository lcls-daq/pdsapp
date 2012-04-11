# constituents for the segment level device production code ONLY

libnames := devapp
libsrcs_devapp := CmdLineTools.cc

CPPFLAGS += -D_ACQIRIS -D_LINUX

ifneq ($(findstring x86_64-linux,$(tgt_arch)),)
tgtnames := opal1kedt pimimageedt phasics 

else
tgtnames :=  evr \
    evrstandalone \
    evrsnoop \
    acq \
    opal1k \
    epicsArch \
    rceProxy \
    encoder \
    bld \
    princeton \
    princetonsim \
    ipimb \
    lusidiag \
    tm6740 \
    pimimage \
    fccd     \
    cspad    \
    xamps    \
    fexamp   \
    gsc16ai  \
    timepix  \
    simcam   \
    cspad2x2 \
    timetool \
    oceanoptics
endif

commonlibs  := pdsdata/xtcdata pdsdata/appdata
commonlibs  += pds/service pds/collection pds/xtc pds/mon pds/vmon pds/utility pds/management pds/client pds/config 
commonlibs  += pdsapp/devapp

tgtsrcs_phasics := phasics.cc
tgtlibs_phasics := $(commonlibs) pds/camera pdsdata/opal1kdata pdsdata/fccddata pdsdata/pulnixdata pdsdata/camdata pdsdata/phasicsdata pds/phasics
tgtincs_phasics += libdc1394/include
tgtlibs_phasics += libdc1394/raw1394
tgtlibs_phasics += libdc1394/dc1394
tgtslib_phasics := $(USRLIBDIR)/rt
CPPFLAGS += -fno-strict-aliasing
CPPFLAGS += -fopenmp
DEFINES += -fopenmp

tgtsrcs_fexamp := fexamp.cc
tgtlibs_fexamp := $(commonlibs) pdsdata/fexampdata pds/fexamp pds/pgp
tgtslib_fexamp := /usr/lib/rt
CPPFLAGS += -fno-strict-aliasing
#CPPFLAGS += -fopenmp
#DEFINES += -fopenmp

tgtsrcs_xamps := xamps.cc
tgtlibs_xamps := $(commonlibs) pdsdata/xampsdata pds/xamps pds/pgp
tgtslib_xamps := /usr/lib/rt
CPPFLAGS += -fno-strict-aliasing
#CPPFLAGS += -fopenmp
#DEFINES += -fopenmp

tgtsrcs_cspad := cspad.cc
tgtlibs_cspad := $(commonlibs) pdsdata/cspaddata pds/cspad pds/pgp
tgtslib_cspad := /usr/lib/rt
CPPFLAGS += -fno-strict-aliasing
#CPPFLAGS += -fopenmp
#DEFINES += -fopenmp

tgtsrcs_cspad2x2 := cspad2x2.cc
tgtlibs_cspad2x2 := $(commonlibs) pdsdata/cspad2x2data pds/cspad2x2 pds/pgp
tgtslib_cspad2x2 := /usr/lib/rt
CPPFLAGS += -fno-strict-aliasing
#CPPFLAGS += -fopenmp
#DEFINES += -fopenmp

tgtsrcs_acq := acq.cc
tgtincs_acq := acqiris
tgtlibs_acq := $(commonlibs) pdsdata/acqdata pds/acqiris acqiris/AqDrv4
tgtslib_acq := /usr/lib/rt

tgtsrcs_ipimb := ipimb.cc
tgtincs_ipimb := ipimb
tgtlibs_ipimb := $(commonlibs) pdsdata/ipimbdata pds/ipimb pdsdata/phasicsdata
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

leutron_libs := pds/camleutron
leutron_libs += leutron/lvsds.34.${ARCHCODE}
leutron_libs += leutron/LvCamDat.34.${ARCHCODE}
leutron_libs += leutron/LvSerialCommunication.34.${ARCHCODE}

edt_libs := pds/camedt edt/pdv

tgtsrcs_opal1k := opal1k.cc 
tgtlibs_opal1k := $(commonlibs) pdsdata/opal1kdata pdsdata/fccddata pdsdata/pulnixdata pdsdata/camdata
tgtlibs_opal1k += pds/camera
tgtlibs_opal1k += $(leutron_libs)
tgtincs_opal1k := leutron/include

tgtsrcs_opal1kedt := opal1kedt.cc 
#tgtlibs_opal1kedt := pdsdata/xtcdata pdsdata/appdata 
tgtlibs_opal1kedt := pdsdata/xtcdata
tgtlibs_opal1kedt += pdsdata/opal1kdata pdsdata/fccddata pdsdata/pulnixdata pdsdata/camdata
tgtlibs_opal1kedt += pds/service pds/collection pds/xtc pds/mon pds/vmon 
tgtlibs_opal1kedt += pds/utility pds/management pds/client pds/config 
tgtlibs_opal1kedt += pds/camera
tgtlibs_opal1kedt += pdsapp/devapp
tgtlibs_opal1kedt += $(edt_libs)
tgtslib_opal1kedt := $(USRLIBDIR)/rt $(USRLIBDIR)/dl
tgtincs_opal1kedt := edt/include


tgtsrcs_timetool := timetool.cc TimeTool.cc
tgtlibs_timetool := $(commonlibs) pdsdata/opal1kdata pdsdata/fccddata pdsdata/pulnixdata pdsdata/camdata
tgtlibs_timetool += pds/camera pds/epicstools epics/ca epics/Com
tgtlibs_timetool += $(leutron_libs)
tgtincs_timetool := leutron/include
tgtincs_timetool += epics/include epics/include/os/Linux

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

tgtsrcs_pimimageedt := pimimageedt.cc 
tgtlibs_pimimageedt := $(commonlibs) pdsdata/opal1kdata pdsdata/fccddata pdsdata/pulnixdata pdsdata/camdata
tgtlibs_pimimageedt += pds/camera
tgtlibs_pimimageedt += $(edt_libs)
tgtslib_pimimageedt := $(USRLIBDIR)/rt $(USRLIBDIR)/dl
tgtincs_pimimageedt := edt/include

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
tgtlibs_princeton := $(commonlibs) pdsdata/pnccddata pdsdata/evrdata pdsdata/princetondata pds/princeton pvcam/pvcam 
tgtlibs_princeton += pdsdata/xampsdata pdsdata/fexampdata pdsdata/cspaddata pdsdata/cspad2x2data pdsdata/lusidata
tgtlibs_princeton += pdsdata/encoderdata pdsdata/ipimbdata pdsdata/princetondata pdsdata/controldata
tgtlibs_princeton += pdsdata/acqdata pdsdata/pnccddata pdsdata/gsc16aidata pdsdata/opal1kdata pdsdata/fccddata pdsdata/pulnixdata pdsdata/camdata pdsdata/timepixdata
tgtlibs_princeton += pdsdata/phasicsdata
tgtlibs_princeton += pdsapp/configdb qt/QtGui qt/QtCore # for accessing configdb
tgtslib_princeton := /usr/lib/rt dl pthread

tgtsrcs_princetonsim := princeton.cc
tgtlibs_princetonsim := $(commonlibs) pdsdata/pnccddata pdsdata/evrdata pdsdata/princetondata pds/princeton pvcam/pvcamtest
tgtlibs_princetonsim += pdsdata/xampsdata pdsdata/fexampdata pdsdata/cspaddata pdsdata/cspad2x2data pdsdata/lusidata
tgtlibs_princetonsim += pdsdata/encoderdata pdsdata/ipimbdata pdsdata/princetondata pdsdata/controldata
tgtlibs_princetonsim += pdsdata/acqdata pdsdata/pnccddata pdsdata/gsc16aidata pdsdata/opal1kdata pdsdata/fccddata pdsdata/pulnixdata pdsdata/camdata pdsdata/timepixdata
tgtlibs_princetonsim += pdsdata/phasicsdata
tgtlibs_princetonsim += pdsapp/configdb qt/QtGui qt/QtCore # for accessing configdb
tgtslib_princetonsim := /usr/lib/rt dl pthread

tgtsrcs_simcam := simcam.cc 
tgtlibs_simcam := $(commonlibs) pdsdata/pulnixdata pdsdata/camdata
tgtslib_simcam := pthread rt

tgtsrcs_gsc16ai := gsc16ai.cc
tgtlibs_gsc16ai := $(commonlibs) pdsdata/gsc16aidata pds/gsc16ai pdsdata/camdata
tgtslib_gsc16ai := /usr/lib/rt

tgtsrcs_timepix := timepix.cc
tgtlibs_timepix := $(commonlibs) pdsdata/timepixdata pds/timepix pdsdata/camdata relaxd/mpxhwrelaxd
tgtslib_timepix := /usr/lib/rt
tgtincs_timepix := relaxd/include/common relaxd/include/src

tgtsrcs_oceanoptics := oceanoptics.cc
tgtlibs_oceanoptics := $(commonlibs) pdsdata/oceanopticsdata pds/oceanoptics pds/oopt
tgtslib_oceanoptics := /usr/lib/rt
