# constituents for the segment level device production code ONLY

libnames := devapp
libsrcs_devapp := CmdLineTools.cc

CPPFLAGS += -D_ACQIRIS -D_LINUX
#CPPFLAGS += -DBLD_DELAY # for tolerating BLD delays up to 0.5 seconds

ifneq ($(findstring x86_64-linux,$(tgt_arch)),)
tgtnames := opal1kedt quartzedt pimimageedt phasics \
  oceanoptics fli andor usdusb
else
tgtnames :=  evr \
    evrstandalone \
    evrsnoop \
    acq \
    opal1k \
    epicsArch \
    rceProxy \
    encoder \
    usdusb \
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
    oceanoptics \
    fli \
    andor
endif

commonlibs  := pdsdata/xtcdata pdsdata/appdata pdsdata/cspaddata pdsdata/cspad2x2data pdsdata/timepixdata pdsdata/camdata  pdsdata/compressdata
commonlibs  += pds/service pds/collection pds/xtc pds/mon pds/vmon pds/utility pds/management pds/client pds/config 
commonlibs  += pdsapp/devapp

#  libconfigdb dependencies
datalibs := pdsdata/xtcdata pdsdata/opal1kdata pdsdata/quartzdata pdsdata/pulnixdata pdsdata/camdata pdsdata/pnccddata pdsdata/evrdata pdsdata/acqdata pdsdata/controldata pdsdata/princetondata pdsdata/ipimbdata pdsdata/encoderdata pdsdata/fccddata pdsdata/lusidata pdsdata/cspaddata pdsdata/xampsdata pdsdata/fexampdata pdsdata/gsc16aidata pdsdata/timepixdata pdsdata/phasicsdata pdsdata/cspad2x2data pdsdata/oceanopticsdata pdsdata/flidata pdsdata/andordata pdsdata/usdusbdata pdsdata/compressdata

tgtsrcs_fexamp := fexamp.cc
tgtlibs_fexamp := $(commonlibs) pdsdata/fexampdata pds/fexamp pds/pgp
tgtslib_fexamp := $(USRLIBDIR)/rt
CPPFLAGS += -fno-strict-aliasing
#CPPFLAGS += -fopenmp
#DEFINES += -fopenmp

tgtsrcs_xamps := xamps.cc
tgtlibs_xamps := $(commonlibs) pdsdata/xampsdata pds/xamps pds/pgp
tgtslib_xamps := $(USRLIBDIR)/rt
CPPFLAGS += -fno-strict-aliasing
#CPPFLAGS += -fopenmp
#DEFINES += -fopenmp

tgtsrcs_cspad := cspad.cc
tgtlibs_cspad := $(commonlibs) pdsdata/cspaddata pds/cspad pds/pgp
tgtslib_cspad := $(USRLIBDIR)/rt
CPPFLAGS += -fno-strict-aliasing
#CPPFLAGS += -fopenmp
#DEFINES += -fopenmp

tgtsrcs_cspad2x2 := cspad2x2.cc
tgtlibs_cspad2x2 := $(commonlibs) pdsdata/cspad2x2data pds/cspad2x2 pds/pgp
tgtslib_cspad2x2 := $(USRLIBDIR)/rt
CPPFLAGS += -fno-strict-aliasing
#CPPFLAGS += -fopenmp
#DEFINES += -fopenmp

tgtsrcs_acq := acq.cc
tgtincs_acq := acqiris
tgtlibs_acq := $(commonlibs) pdsdata/acqdata pds/acqiris acqiris/AqDrv4
tgtslib_acq := $(USRLIBDIR)/rt

tgtsrcs_ipimb := ipimb.cc
tgtincs_ipimb := ipimb
tgtlibs_ipimb := $(commonlibs) pdsdata/ipimbdata pds/ipimb pdsdata/phasicsdata
tgtslib_ipimb := $(USRLIBDIR)/rt

tgtsrcs_lusidiag := lusidiag.cc
tgtincs_lusidiag := lusidiag
tgtlibs_lusidiag := $(commonlibs) pdsdata/ipimbdata pdsdata/lusidata pds/ipimb
tgtslib_lusidiag := $(USRLIBDIR)/rt

tgtsrcs_encoder := encoder.cc
tgtincs_encoder := encoder
tgtlibs_encoder := $(commonlibs) pdsdata/encoderdata pds/encoder
tgtslib_encoder := $(USRLIBDIR)/rt

tgtsrcs_usdusb := usdusb.cc
tgtincs_usdusb := usdusb4/include libusb/include/libusb-1.0
tgtlibs_usdusb := $(commonlibs) pdsdata/usdusbdata pds/usdusb usdusb4/usdusb4 libusb/usb-1.0
tgtslib_usdusb := $(USRLIBDIR)/rt 

tgtsrcs_evr := evr.cc
tgtincs_evr := evgr
tgtlibs_evr := pdsdata/xtcdata pdsdata/evrdata
tgtlibs_evr += evgr/evr evgr/evg 
tgtlibs_evr += $(commonlibs) pds/evgr 
tgtslib_evr := $(USRLIBDIR)/rt

tgtsrcs_evrstandalone := evrstandalone.cc
tgtincs_evrstandalone := evgr
tgtlibs_evrstandalone := $(commonlibs) pdsdata/evrdata
tgtlibs_evrstandalone += evgr/evr evgr/evg 
tgtlibs_evrstandalone += pds/evgr
tgtslib_evrstandalone := $(USRLIBDIR)/rt

tgtsrcs_evrsnoop := evrsnoop.cc
tgtincs_evrsnoop := evgr
tgtlibs_evrsnoop := $(commonlibs) pdsdata/evrdata
tgtlibs_evrsnoop += evgr/evr evgr/evg 
tgtlibs_evrsnoop += pds/evgr
tgtslib_evrsnoop := $(USRLIBDIR)/rt

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

cam_libs := pdsdata/opal1kdata pdsdata/quartzdata pdsdata/fccddata pdsdata/pulnixdata pdsdata/camdata pdsdata/cspaddata pdsdata/cspad2x2data pdsdata/timepixdata

tgtsrcs_opal1k := opal1k.cc 
tgtlibs_opal1k := $(commonlibs) $(cam_libs)
tgtlibs_opal1k += pds/camera
tgtlibs_opal1k += $(leutron_libs)
tgtincs_opal1k := leutron/include

tgtsrcs_opal1kedt := opal1kedt.cc 
tgtlibs_opal1kedt := pdsdata/xtcdata
tgtlibs_opal1kedt += $(cam_libs)
tgtlibs_opal1kedt += pds/service pds/collection pds/xtc pds/mon pds/vmon 
tgtlibs_opal1kedt += pds/utility pds/management pds/client pds/config 
tgtlibs_opal1kedt += pds/camera
tgtlibs_opal1kedt += pdsapp/devapp
tgtlibs_opal1kedt += $(edt_libs)
tgtslib_opal1kedt := $(USRLIBDIR)/rt/rt $(USRLIBDIR)/rt/dl
tgtincs_opal1kedt := edt/include

tgtsrcs_quartzedt := quartzedt.cc 
tgtlibs_quartzedt := pdsdata/xtcdata
tgtlibs_quartzedt += $(cam_libs)
tgtlibs_quartzedt += pds/service pds/collection pds/xtc pds/mon pds/vmon 
tgtlibs_quartzedt += pds/utility pds/management pds/client pds/config 
tgtlibs_quartzedt += pds/camera
tgtlibs_quartzedt += pdsapp/devapp
tgtlibs_quartzedt += $(edt_libs)
tgtslib_quartzedt := $(USRLIBDIR)/rt/rt $(USRLIBDIR)/rt/dl
tgtincs_quartzedt := edt/include

tgtsrcs_timetool := timetool.cc TimeTool.cc
tgtlibs_timetool := $(commonlibs) $(cam_libs)
tgtlibs_timetool += pds/camera pds/epicstools epics/ca epics/Com
tgtlibs_timetool += $(leutron_libs)
tgtincs_timetool := leutron/include
tgtincs_timetool += epics/include epics/include/os/Linux

tgtsrcs_tm6740 := tm6740.cc 
tgtlibs_tm6740 := $(commonlibs) $(cam_libs)
tgtlibs_tm6740 += pds/camera
tgtlibs_tm6740 += $(leutron_libs)
tgtincs_tm6740 := leutron/include

tgtsrcs_pimimage := pimimage.cc 
tgtlibs_pimimage := $(commonlibs) $(cam_libs)
tgtlibs_pimimage += pds/camera
tgtlibs_pimimage += $(leutron_libs)
tgtincs_pimimage := leutron/include

tgtsrcs_pimimageedt := pimimageedt.cc 
tgtlibs_pimimageedt := $(commonlibs) $(cam_libs)
tgtlibs_pimimageedt += pds/camera
tgtlibs_pimimageedt += $(edt_libs)
tgtslib_pimimageedt := $(USRLIBDIR)/rt/rt $(USRLIBDIR)/rt/dl
tgtincs_pimimageedt := edt/include

tgtsrcs_fccd := fccd.cc
tgtlibs_fccd := $(commonlibs) $(cam_libs)
tgtlibs_fccd += pds/camera
tgtlibs_fccd += $(leutron_libs)
tgtincs_fccd := leutron/include

tgtsrcs_phasics := phasics.cc
tgtlibs_phasics := $(commonlibs) $(cam_libs) pds/phasics pdsdata/phasicsdata
tgtincs_phasics += libdc1394/include
tgtlibs_phasics += libdc1394/raw1394
tgtlibs_phasics += libdc1394/dc1394
tgtslib_phasics := $(USRLIBDIR)/rt/rt
CPPFLAGS += -fno-strict-aliasing
CPPFLAGS += -fopenmp
DEFINES += -fopenmp

tgtsrcs_epicsArch := epicsArch.cc
tgtlibs_epicsArch := $(commonlibs) pdsdata/epics pds/epicsArch epics/ca epics/Com
tgtslib_epicsArch := $(USRLIBDIR)/rt

tgtsrcs_bld := bld.cc 
tgtlibs_bld := $(commonlibs) pdsdata/evrdata
tgtslib_bld := $(USRLIBDIR)/rt

tgtsrcs_rceProxy := rceProxy.cc
tgtlibs_rceProxy := $(commonlibs) pdsdata/pnccddata pds/rceProxy
tgtslib_rceProxy := $(USRLIBDIR)/rt

tgtsrcs_princeton := princeton.cc
tgtlibs_princeton := $(commonlibs) pds/princeton pvcam/pvcam 
tgtlibs_princeton += $(datalibs) 
tgtlibs_princeton += pdsapp/configdb qt/QtGui qt/QtCore # for accessing configdb
tgtslib_princeton := $(USRLIBDIR)/rt dl pthread

tgtsrcs_princetonsim := princeton.cc
tgtlibs_princetonsim := $(commonlibs) pds/princeton pvcam/pvcamtest
tgtlibs_princetonsim += $(datalibs) 
tgtlibs_princetonsim += pdsapp/configdb qt/QtGui qt/QtCore # for accessing configdb
tgtslib_princetonsim := $(USRLIBDIR)/rt dl pthread

tgtsrcs_simcam := simcam.cc 
tgtlibs_simcam := $(commonlibs) pdsdata/pulnixdata pdsdata/camdata
tgtslib_simcam := pthread rt

tgtsrcs_gsc16ai := gsc16ai.cc
tgtlibs_gsc16ai := $(commonlibs) pdsdata/gsc16aidata pds/gsc16ai pdsdata/camdata
tgtslib_gsc16ai := $(USRLIBDIR)/rt

tgtsrcs_timepix := timepix.cc
tgtlibs_timepix := $(commonlibs) pdsdata/timepixdata pds/timepix pdsdata/camdata relaxd/mpxhwrelaxd
tgtslib_timepix := $(USRLIBDIR)/rt
tgtincs_timepix := relaxd/include/common relaxd/include/src

tgtsrcs_oceanoptics := oceanoptics.cc
tgtlibs_oceanoptics := $(commonlibs) pdsdata/oceanopticsdata pds/oceanoptics pds/oopt
tgtslib_oceanoptics := ${USRLIBDIR}/rt

tgtsrcs_fli := fli.cc
tgtlibs_fli := $(commonlibs) 
tgtlibs_fli += $(datalibs)
tgtlibs_fli += pdsapp/configdb qt/QtGui qt/QtCore # for accessing configdb
tgtlibs_fli += pds/fli fli/flisdk
tgtslib_fli := ${USRLIBDIR}/rt ${USRLIBDIR}/dl ${USRLIBDIR}/pthread 

tgtsrcs_andor := andor.cc
tgtlibs_andor := $(commonlibs) 
tgtlibs_andor += $(datalibs)
tgtlibs_andor += pdsapp/configdb qt/QtGui qt/QtCore # for accessing configdb
tgtlibs_andor += pdsdata/andordata pds/pdsandor andor/andor
tgtslib_andor := ${USRLIBDIR}/rt ${USRLIBDIR}/dl ${USRLIBDIR}/pthread 
