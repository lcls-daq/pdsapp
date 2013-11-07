# constituents for the segment level device production code ONLY

libnames := devapp
libsrcs_devapp := CmdLineTools.cc
libincs_devapp := pdsdata/include

CPPFLAGS += -D_ACQIRIS -D_LINUX
#CPPFLAGS += -DBLD_DELAY # for tolerating BLD delays up to 0.5 seconds

ifneq ($(findstring x86_64,$(tgt_arch)),)
tgtnames := \
  oceanoptics fli andor usdusb camedt simcam \
  bld evr
  ifeq ($(build_extra),$(true))
    tgtnames += phasics
  endif
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
    gsc16ai  \
    timepix  \
    rayonix  \
    simcam   \
    cspad2x2 \
    oceanoptics \
    fli \
    andor \
    cam \
    imp \
    pnccd
  ifeq ($(build_extra),$(true))
    tgtnames += xamps fexamp
  endif
endif

commonlibs  := pdsdata/xtcdata pdsdata/appdata pdsdata/psddl_pdsdata
commonlibs  += pds/service pds/collection pds/xtc pds/mon pds/vmon pds/utility pds/management pds/client pds/config 
commonlibs  += pdsapp/devapp

tgtsrcs_fexamp := fexamp.cc
tgtlibs_fexamp := $(commonlibs) pds/fexamp pds/pgp
tgtslib_fexamp := $(USRLIBDIR)/rt
tgtincs_fexamp := pdsdata/include
CPPFLAGS += -fno-strict-aliasing
#CPPFLAGS += -fopenmp
#DEFINES += -fopenmp

tgtsrcs_xamps := xamps.cc
tgtlibs_xamps := $(commonlibs) pds/xamps pds/pgp
tgtslib_xamps := $(USRLIBDIR)/rt
tgtincs_xamps := pdsdata/include
CPPFLAGS += -fno-strict-aliasing
#CPPFLAGS += -fopenmp
#DEFINES += -fopenmp

tgtsrcs_imp := imp.cc
tgtlibs_imp := $(commonlibs) pds/imp pds/pgp pds/configdata
tgtslib_imp := $(USRLIBDIR)/rt
tgtincs_imp := pdsdata/include ndarray/include
CPPFLAGS += -fno-strict-aliasing
#CPPFLAGS += -fopenmp
#DEFINES += -fopenmp

tgtsrcs_pnccd := pnccd.cc
tgtlibs_pnccd := $(commonlibs) pds/pnccd pds/pnccdFrameV0 pds/pgp pds/configdata
tgtslib_pnccd := $(USRLIBDIR)/rt
tgtincs_pnccd := pdsdata/include ndarray/include
CPPFLAGS += -fno-strict-aliasing
#CPPFLAGS += -fopenmp
#DEFINES += -fopenmp

tgtsrcs_cspad := cspad.cc
tgtlibs_cspad := $(commonlibs) pds/cspad pds/pgp
tgtlibs_cspad += pds/clientcompress pdsdata/compressdata pds/configdata
tgtslib_cspad := $(USRLIBDIR)/rt
tgtincs_cspad := pdsdata/include ndarray/include
CPPFLAGS += -fno-strict-aliasing
#CPPFLAGS += -fopenmp
#DEFINES += -fopenmp

tgtsrcs_cspad2x2 := cspad2x2.cc
tgtlibs_cspad2x2 := $(commonlibs) pds/cspad2x2 pds/pgp
tgtlibs_cspad2x2 += pds/clientcompress pds/configdata pdsdata/compressdata
tgtslib_cspad2x2 := $(USRLIBDIR)/rt
tgtincs_cspad2x2 := pdsdata/include ndarray/include
CPPFLAGS += -fno-strict-aliasing
#CPPFLAGS += -fopenmp
#DEFINES += -fopenmp

tgtsrcs_acq := acq.cc
tgtincs_acq := acqiris pdsdata/include ndarray/include
tgtlibs_acq := $(commonlibs) pds/acqiris acqiris/AqDrv4
tgtslib_acq := $(USRLIBDIR)/rt

tgtsrcs_ipimb := ipimb.cc
tgtincs_ipimb := ipimb pdsdata/include ndarray/include
tgtlibs_ipimb := $(commonlibs) pds/ipimb
tgtslib_ipimb := $(USRLIBDIR)/rt

tgtsrcs_lusidiag := lusidiag.cc
tgtincs_lusidiag := lusidiag pdsdata/include ndarray/include
tgtlibs_lusidiag := $(commonlibs) pds/ipimb
tgtslib_lusidiag := $(USRLIBDIR)/rt

tgtsrcs_encoder := encoder.cc
tgtincs_encoder := encoder pdsdata/include ndarray/include
tgtlibs_encoder := $(commonlibs) pds/encoder
tgtslib_encoder := $(USRLIBDIR)/rt

tgtsrcs_usdusb := usdusb.cc
tgtincs_usdusb := usdusb4/include libusb/include/libusb-1.0 
tgtincs_usdusb += pdsdata/include ndarray/include
tgtlibs_usdusb := $(commonlibs) pds/usdusb usdusb4/usdusb4 libusb/usb-1.0
tgtslib_usdusb := $(USRLIBDIR)/rt 

tgtsrcs_evr := evr.cc
tgtincs_evr := evgr pdsdata/include ndarray/include boost/include
tgtlibs_evr := pdsdata/xtcdata pdsdata/psddl_pdsdata pds/configdata
tgtlibs_evr += evgr/evr evgr/evg 
tgtlibs_evr += $(commonlibs) pds/evgr 
tgtslib_evr := $(USRLIBDIR)/rt

tgtsrcs_evrstandalone := evrstandalone.cc
tgtincs_evrstandalone := evgr pdsdata/include
tgtlibs_evrstandalone := $(commonlibs) pds/configdata
tgtlibs_evrstandalone += evgr/evr evgr/evg 
tgtlibs_evrstandalone += pds/evgr
tgtslib_evrstandalone := $(USRLIBDIR)/rt

tgtsrcs_evrsnoop := evrsnoop.cc
tgtincs_evrsnoop := evgr pdsdata/include
tgtlibs_evrsnoop := $(commonlibs) pds/configdata
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

cam_libs := pdsdata/psddl_pdsdata

tgtsrcs_cam := cam.cc 
tgtlibs_cam := $(commonlibs) $(cam_libs)
tgtlibs_cam += pds/camera pds/epicstools epics/ca epics/Com
tgtlibs_cam += pds/clientcompress pdsdata/compressdata pds/configdata
tgtlibs_cam += $(leutron_libs)
tgtincs_cam := leutron/include pdsdata/include ndarray/include 
tgtincs_cam += epics/include epics/include/os/Linux

tgtsrcs_camedt := camedt.cc 
tgtlibs_camedt := pdsdata/xtcdata pdsdata/compressdata
tgtlibs_camedt += $(cam_libs)
tgtlibs_camedt += pds/service pds/collection pds/xtc pds/mon pds/vmon 
tgtlibs_camedt += pds/utility pds/management pds/client pds/config
tgtlibs_camedt += pds/camera pds/epicstools epics/ca epics/Com
tgtlibs_camedt += pds/clientcompress pdsdata/compressdata pds/configdata
tgtlibs_camedt += pdsapp/devapp
tgtlibs_camedt += $(edt_libs)
tgtslib_camedt := $(USRLIBDIR)/rt/rt $(USRLIBDIR)/rt/dl
tgtincs_camedt := edt/include pdsdata/include ndarray/include 
tgtincs_camedt += epics/include epics/include/os/Linux

tgtsrcs_fccd := fccd.cc
tgtlibs_fccd := $(commonlibs) $(cam_libs)
tgtlibs_fccd += pds/camera pds/configdata
tgtlibs_fccd += $(leutron_libs)
tgtincs_fccd := leutron/include pdsdata/include ndarray/include

tgtsrcs_phasics := phasics.cc
tgtlibs_phasics := $(commonlibs) $(cam_libs) pds/phasics
tgtincs_phasics += libdc1394/include pdsdata/include
tgtlibs_phasics += libdc1394/raw1394
tgtlibs_phasics += libdc1394/dc1394
tgtslib_phasics := $(USRLIBDIR)/rt/rt
CPPFLAGS += -fno-strict-aliasing
CPPFLAGS += -fopenmp
DEFINES += -fopenmp

tgtsrcs_epicsArch := epicsArch.cc
tgtlibs_epicsArch := $(commonlibs) pds/epicsArch epics/ca epics/Com
tgtslib_epicsArch := $(USRLIBDIR)/rt
tgtincs_epicsArch := pdsdata/include ndarray/include

tgtsrcs_bld := bld.cc 
tgtlibs_bld := $(commonlibs) 
tgtlibs_bld += pds/clientcompress pdsdata/compressdata
tgtslib_bld := $(USRLIBDIR)/rt
tgtincs_bld := pdsdata/include ndarray/include 

tgtsrcs_princeton := princeton.cc
tgtlibs_princeton := $(commonlibs) pds/princeton pvcam/pvcam 
tgtlibs_princeton += pdsapp/configdb pds/configdata $(qtlibdir) # for accessing configdb
tgtslib_princeton := $(USRLIBDIR)/rt dl pthread
tgtincs_princeton := pdsdata/include ndarray/include

tgtsrcs_princetonsim := princeton.cc
tgtlibs_princetonsim := $(commonlibs) pds/princeton pvcam/pvcamtest
tgtlibs_princetonsim += pdsapp/configdb pds/configdata $(qtlibdir) # for accessing configdb
tgtslib_princetonsim := $(USRLIBDIR)/rt dl pthread
tgtincs_princetonsim := pdsdata/include ndarray/include

tgtsrcs_simcam := simcam.cc 
tgtsrcs_simcam += SimCam.cc
tgtlibs_simcam := $(commonlibs) $(cam_libs)
tgtlibs_simcam += pds/clientcompress pdsdata/compressdata pds/configdata
tgtlibs_simcam += pds/camera pds/epicstools epics/ca epics/Com
tgtslib_simcam := pthread rt dl
tgtincs_simcam := pdsdata/include ndarray/include  boost/include


tgtsrcs_gsc16ai := gsc16ai.cc
tgtlibs_gsc16ai := $(commonlibs) pds/gsc16ai
tgtslib_gsc16ai := $(USRLIBDIR)/rt
tgtincs_gsc16ai := pdsdata/include ndarray/include

tgtsrcs_timepix := timepix.cc
tgtlibs_timepix := $(commonlibs) pds/timepix relaxd/mpxhwrelaxd pds/configdata
tgtslib_timepix := $(USRLIBDIR)/rt
tgtincs_timepix := relaxd/include/common relaxd/include/src pdsdata/include ndarray/include

tgtsrcs_rayonix := rayonix.cc
tgtlibs_rayonix := $(commonlibs) pds/rayonix
tgtslib_rayonix := $(USRLIBDIR)/rt
tgtincs_rayonix := pdsdata/include ndarray/include

tgtsrcs_oceanoptics := oceanoptics.cc
tgtlibs_oceanoptics := $(commonlibs) pds/oceanoptics pds/oopt
tgtslib_oceanoptics := ${USRLIBDIR}/rt
tgtincs_oceanoptics := pdsdata/include ndarray/include

tgtsrcs_fli := fli.cc
tgtlibs_fli := $(commonlibs) 
tgtlibs_fli += pdsapp/configdb $(qtlibdir) # for accessing configdb
tgtlibs_fli += pds/fli fli/flisdk pds/configdata
tgtslib_fli := ${USRLIBDIR}/rt ${USRLIBDIR}/dl ${USRLIBDIR}/pthread 
tgtincs_fli := pdsdata/include ndarray/include

tgtsrcs_andor := andor.cc
tgtlibs_andor := $(commonlibs) 
tgtlibs_andor += pdsapp/configdb $(qtlibdir) # for accessing configdb
tgtlibs_andor += pds/pdsandor pds/configdata andor/andor
tgtslib_andor := ${USRLIBDIR}/rt ${USRLIBDIR}/dl ${USRLIBDIR}/pthread 
tgtincs_andor := pdsdata/include ndarray/include

