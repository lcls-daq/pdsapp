# constituents for the segment level device production code ONLY

libnames := devapp
libsrcs_devapp := CmdLineTools.cc

CPPFLAGS += -D_ACQIRIS -D_LINUX
#CPPFLAGS += -DBLD_DELAY # for tolerating BLD delays up to 0.5 seconds

ifneq ($(findstring x86_64,$(tgt_arch)),)
tgtnames := phasics \
  oceanoptics fli andor usdusb camedt simcam \
  bld evr
else
tgtnames :=  evr \
    evrstandalone \
    evrsnoop \
    acq \
    epicsArch \
    encoder \
    usdusb \
    bld \
    princeton \
    princetonsim \
    ipimb \
    lusidiag \
    fccd     \
    cspad    \
    xamps    \
    fexamp   \
    gsc16ai  \
    timepix  \
    simcam   \
    cspad2x2 \
    oceanoptics \
    fli \
    andor \
    cam \
    imp \
    pnccd
endif

commonlibs  := pdsdata/xtcdata pdsdata/appdata pdsdata/cspaddata pdsdata/cspad2x2data pdsdata/timepixdata pdsdata/camdata
commonlibs  += pds/service pds/collection pds/xtc pds/mon pds/vmon pds/utility pds/management pds/client pds/config 
commonlibs  += pdsapp/devapp pdsdata/aliasdata

#  Get datalibs macro
include ../../pdsdata/packages.mk

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

tgtsrcs_imp := imp.cc
tgtlibs_imp := $(commonlibs) pdsdata/impdata pds/imp pds/pgp
tgtslib_imp := $(USRLIBDIR)/rt
CPPFLAGS += -fno-strict-aliasing
#CPPFLAGS += -fopenmp
#DEFINES += -fopenmp

tgtsrcs_pnccd := pnccd.cc
tgtlibs_pnccd := $(commonlibs) pdsdata/pnccddata pds/pnccd pds/pnccdFrameV0 pds/pgp
tgtslib_pnccd := $(USRLIBDIR)/rt
CPPFLAGS += -fno-strict-aliasing
#CPPFLAGS += -fopenmp
#DEFINES += -fopenmp

tgtsrcs_cspad := cspad.cc
tgtlibs_cspad := $(commonlibs) pdsdata/cspaddata pds/cspad pds/pgp
tgtlibs_cspad += pds/clientcompress pdsdata/compressdata
tgtslib_cspad := $(USRLIBDIR)/rt
CPPFLAGS += -fno-strict-aliasing
#CPPFLAGS += -fopenmp
#DEFINES += -fopenmp

tgtsrcs_cspad2x2 := cspad2x2.cc
tgtlibs_cspad2x2 := $(commonlibs) pdsdata/cspad2x2data pds/cspad2x2 pds/pgp
tgtlibs_cspad2x2 += pds/clientcompress pdsdata/compressdata
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

cam_libs := pdsdata/opal1kdata pdsdata/quartzdata pdsdata/fccddata pdsdata/pulnixdata pdsdata/camdata pdsdata/cspaddata pdsdata/cspad2x2data pdsdata/timepixdata pdsdata/orcadata

tgtsrcs_cam := cam.cc 
tgtlibs_cam := $(commonlibs) $(cam_libs)
tgtlibs_cam += pds/camera pds/epicstools epics/ca epics/Com
tgtlibs_cam += pds/clientcompress pdsdata/compressdata
tgtlibs_cam += $(leutron_libs)
tgtincs_cam := leutron/include
tgtincs_cam += epics/include epics/include/os/Linux

tgtsrcs_camedt := camedt.cc 
tgtlibs_camedt := pdsdata/xtcdata pdsdata/compressdata
tgtlibs_camedt += $(cam_libs)
tgtlibs_camedt += pds/service pds/collection pds/xtc pds/mon pds/vmon 
tgtlibs_camedt += pds/utility pds/management pds/client pds/config pdsdata/aliasdata
tgtlibs_camedt += pds/camera pds/epicstools epics/ca epics/Com
tgtlibs_camedt += pds/clientcompress pdsdata/compressdata
tgtlibs_camedt += pdsapp/devapp
tgtlibs_camedt += $(edt_libs)
tgtslib_camedt := $(USRLIBDIR)/rt/rt $(USRLIBDIR)/rt/dl
tgtincs_camedt := edt/include
tgtincs_camedt += epics/include epics/include/os/Linux

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
tgtlibs_bld := $(commonlibs) pdsdata/evrdata pdsdata/acqdata pdsdata/ipimbdata pdsdata/pulnixdata pdsdata/lusidata
tgtlibs_bld += pds/clientcompress pdsdata/compressdata
tgtslib_bld := $(USRLIBDIR)/rt

tgtsrcs_princeton := princeton.cc
tgtlibs_princeton := $(commonlibs) pds/princeton pvcam/pvcam 
tgtlibs_princeton += $(datalibs) 
tgtlibs_princeton += pdsapp/configdb $(qtlibdir) # for accessing configdb
tgtslib_princeton := $(USRLIBDIR)/rt dl pthread

tgtsrcs_princetonsim := princeton.cc
tgtlibs_princetonsim := $(commonlibs) pds/princeton pvcam/pvcamtest
tgtlibs_princetonsim += $(datalibs) 
tgtlibs_princetonsim += pdsapp/configdb $(qtlibdir) # for accessing configdb
tgtslib_princetonsim := $(USRLIBDIR)/rt dl pthread

tgtsrcs_simcam := simcam.cc 
tgtsrcs_simcam += SimCam.cc
tgtlibs_simcam := $(commonlibs) $(cam_libs)
tgtlibs_simcam += pdsdata/impdata
tgtlibs_simcam += pds/clientcompress pdsdata/compressdata
tgtlibs_simcam += pds/camera pds/epicstools epics/ca epics/Com
tgtslib_simcam := pthread rt dl


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
tgtlibs_fli += pdsapp/configdb $(qtlibdir) # for accessing configdb
tgtlibs_fli += pds/fli fli/flisdk
tgtslib_fli := ${USRLIBDIR}/rt ${USRLIBDIR}/dl ${USRLIBDIR}/pthread 

tgtsrcs_andor := andor.cc
tgtlibs_andor := $(commonlibs) 
tgtlibs_andor += $(datalibs)
tgtlibs_andor += pdsapp/configdb $(qtlibdir) # for accessing configdb
tgtlibs_andor += pdsdata/andordata pds/pdsandor andor/andor
tgtslib_andor := ${USRLIBDIR}/rt ${USRLIBDIR}/dl ${USRLIBDIR}/pthread 

