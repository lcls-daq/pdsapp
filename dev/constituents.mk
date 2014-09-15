# constituents for the segment level device production code ONLY

CPPFLAGS += -D_ACQIRIS -D_LINUX
CPPFLAGS += -fno-strict-aliasing
#CPPFLAGS += -DBLD_DELAY # for tolerating BLD delays up to 0.5 seconds
#CPPFLAGS += -fopenmp
#DEFINES += -fopenmp

tgtnames := evr evrstandalone evrsnoop
tgtnames += epicsArch bld cspad cspad2x2
tgtnames += imp pnccd epix epixsampler epix10k genericpgp
tgtnames += simcam
tgtnames += ipimb lusidiag
tgtnames += rayonix udpcam
tgtnames += oceanoptics fli andor

ifneq ($(findstring x86_64,$(tgt_arch)),)
tgtnames += camedt
  ifeq ($(build_extra),$(true))
    tgtnames += phasics xamps fexamp
  endif
else
tgtnames +=  acq \
    encoder \
    princeton \
    princetonsim \
    gsc16ai  \
    cam \
    usdusb
endif

ifneq ($(findstring x86_64-rhel6,$(tgt_arch)),)
tgtnames += pimax
endif

commonlibs  := pdsdata/xtcdata pdsdata/appdata pdsdata/psddl_pdsdata
commonlibs  += pds/service pds/collection pds/xtc pds/mon pds/vmon pds/utility pds/management pds/client
commonlibs  += pds/config pds/configdbc pds/confignfs pds/configsql
commonlibs  += offlinedb/mysqlclient

#commonslib  := $(USRLIBDIR)/rt $(USRLIBDIR)/mysql/mysqlclient
commonslib  := $(USRLIBDIR)/rt

tgtsrcs_fexamp := fexamp.cc
tgtlibs_fexamp := $(commonlibs) pds/fexamp pds/pgp
tgtslib_fexamp := $(commonslib)
tgtincs_fexamp := pdsdata/include

tgtsrcs_xamps := xamps.cc
tgtlibs_xamps := $(commonlibs) pds/xamps pds/pgp
tgtslib_xamps := $(commonslib)
tgtincs_xamps := pdsdata/include

tgtsrcs_epixsampler := epixsampler.cc
tgtlibs_epixsampler := $(commonlibs) pds/epixsampler pds/pgp pds/configdata
tgtslib_epixsampler := $(commonslib)
tgtincs_epixsampler := pdsdata/include ndarray/include boost/include 

tgtsrcs_epix := epix.cc
tgtlibs_epix := $(commonlibs) pds/epix pds/pgp pds/configdata
tgtslib_epix := $(commonslib)
tgtincs_epix := pdsdata/include ndarray/include boost/include 

tgtsrcs_epix10k := epix10k.cc
tgtlibs_epix10k := $(commonlibs) pds/epix10k pds/pgp pds/configdata
tgtslib_epix10k := $(commonslib)
tgtincs_epix10k := pdsdata/include ndarray/include boost/include 

tgtsrcs_genericpgp := genericpgp.cc
tgtlibs_genericpgp := $(commonlibs) pds/genericpgp pds/pgp pds/configdata
tgtslib_genericpgp := $(commonslib)
tgtincs_genericpgp := pdsdata/include ndarray/include boost/include 


tgtsrcs_imp := imp.cc
tgtlibs_imp := $(commonlibs) pds/imp pds/pgp pds/configdata
tgtslib_imp := $(commonslib)
tgtincs_imp := pdsdata/include ndarray/include boost/include 

tgtsrcs_pnccd := pnccd.cc
tgtlibs_pnccd := $(commonlibs) pds/pnccd pds/pnccdFrameV0 pds/pgp pds/configdata
tgtslib_pnccd := $(commonslib)
tgtincs_pnccd := pdsdata/include ndarray/include boost/include 

tgtsrcs_cspad := cspad.cc
tgtlibs_cspad := $(commonlibs) pds/cspad pds/pgp
tgtlibs_cspad += pds/clientcompress pds/pnccdFrameV0 pdsdata/compressdata pds/configdata
tgtslib_cspad := $(commonslib) $(USRLIBDIR)/dl
tgtincs_cspad := pdsdata/include ndarray/include boost/include 

tgtsrcs_cspad2x2 := cspad2x2.cc
tgtlibs_cspad2x2 := $(commonlibs) pds/cspad2x2 pds/pgp
tgtlibs_cspad2x2 += pds/clientcompress pds/pnccdFrameV0 pds/configdata pdsdata/compressdata
tgtslib_cspad2x2 := $(commonslib)
tgtincs_cspad2x2 := pdsdata/include ndarray/include boost/include 

tgtsrcs_acq := acq.cc
tgtincs_acq := acqiris pdsdata/include ndarray/include boost/include 
tgtlibs_acq := $(commonlibs) pds/acqiris acqiris/AqDrv4
tgtslib_acq := $(commonslib)

tgtsrcs_ipimb := ipimb.cc
tgtincs_ipimb := ipimb pdsdata/include ndarray/include boost/include 
tgtlibs_ipimb := $(commonlibs) pds/ipimb
tgtslib_ipimb := $(commonslib)

tgtsrcs_lusidiag := lusidiag.cc
tgtincs_lusidiag := lusidiag pdsdata/include ndarray/include boost/include 
tgtlibs_lusidiag := $(commonlibs) pds/ipimb
tgtslib_lusidiag := $(commonslib)

tgtsrcs_encoder := encoder.cc
tgtincs_encoder := encoder pdsdata/include ndarray/include boost/include 
tgtlibs_encoder := $(commonlibs) pds/encoder
tgtslib_encoder := $(commonslib)

tgtsrcs_usdusb := usdusb.cc
tgtincs_usdusb := usdusb4/include libusb/include/libusb-1.0 
tgtincs_usdusb += pdsdata/include ndarray/include boost/include 
tgtlibs_usdusb := $(commonlibs) pds/usdusb usdusb4/usdusb4 libusb/usb-1.0
tgtslib_usdusb := $(commonslib) 

tgtsrcs_evr := evr.cc
tgtincs_evr := evgr pdsdata/include ndarray/include boost/include  
tgtlibs_evr := pdsdata/xtcdata pdsdata/psddl_pdsdata pds/configdata
tgtlibs_evr += evgr/evr evgr/evg 
tgtlibs_evr += $(commonlibs) pds/evgr 
tgtslib_evr := $(commonslib)

tgtsrcs_evrstandalone := evrstandalone.cc
tgtincs_evrstandalone := evgr pdsdata/include
tgtlibs_evrstandalone := $(commonlibs) pds/configdata
tgtlibs_evrstandalone += evgr/evr evgr/evg 
tgtlibs_evrstandalone += pds/evgr
tgtslib_evrstandalone := $(commonslib)

tgtsrcs_evrsnoop := evrsnoop.cc
tgtincs_evrsnoop := evgr pdsdata/include
tgtlibs_evrsnoop := $(commonlibs) pds/configdata
tgtlibs_evrsnoop += evgr/evr evgr/evg 
tgtlibs_evrsnoop += pds/evgr
tgtslib_evrsnoop := $(commonslib)

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
tgtlibs_cam += pds/clientcompress pds/pnccdFrameV0 pdsdata/compressdata pds/configdata
tgtlibs_cam += pds/service pdsdata/xtcdata
tgtlibs_cam += $(leutron_libs)
tgtslib_cam := $(commonslib)
tgtincs_cam := leutron/include pdsdata/include ndarray/include boost/include  
tgtincs_cam += epics/include epics/include/os/Linux

tgtsrcs_camedt := camedt.cc 
tgtlibs_camedt := $(commonlibs) $(cam_libs)
tgtlibs_camedt += pds/camera pds/epicstools epics/ca epics/Com
tgtlibs_camedt += pds/clientcompress pds/pnccdFrameV0 pdsdata/compressdata pds/configdata
tgtlibs_camedt += $(edt_libs)
tgtslib_camedt := $(commonslib) $(USRLIBDIR)/dl
tgtincs_camedt := edt/include pdsdata/include ndarray/include boost/include  
tgtincs_camedt += epics/include epics/include/os/Linux

tgtsrcs_phasics := phasics.cc
tgtlibs_phasics := $(commonlibs) $(cam_libs) pds/phasics
tgtincs_phasics += libdc1394/include pdsdata/include
tgtlibs_phasics += libdc1394/raw1394
tgtlibs_phasics += libdc1394/dc1394
tgtslib_phasics := $(commonslib)/rt

tgtsrcs_epicsArch := epicsArch.cc
tgtlibs_epicsArch := $(commonlibs) pds/epicsArch epics/ca epics/Com
tgtslib_epicsArch := $(commonslib)
tgtincs_epicsArch := pdsdata/include ndarray/include boost/include 

tgtsrcs_bld := bld.cc 
tgtlibs_bld := $(commonlibs) 
tgtlibs_bld += pds/clientcompress pds/pnccdFrameV0 pdsdata/compressdata
tgtslib_bld := $(commonslib)
tgtincs_bld := pdsdata/include ndarray/include boost/include  

tgtsrcs_princeton := princeton.cc
tgtlibs_princeton := $(commonlibs) pds/princeton pvcam/pvcam 
tgtlibs_princeton += pdsapp/configdb pds/configdata
tgtslib_princeton := $(commonslib) dl pthread
tgtincs_princeton := pdsdata/include ndarray/include boost/include 

tgtsrcs_princetonsim := princeton.cc
tgtlibs_princetonsim := $(commonlibs) pds/princeton pvcam/pvcamtest
tgtlibs_princetonsim += pdsapp/configdb pds/configdata
tgtslib_princetonsim := $(commonslib) dl pthread
tgtincs_princetonsim := pdsdata/include ndarray/include boost/include 

tgtsrcs_simcam := simcam.cc 
tgtsrcs_simcam += SimCam.cc
tgtlibs_simcam := $(commonlibs) $(cam_libs)
tgtlibs_simcam += pds/clientcompress pds/pnccdFrameV0 pdsdata/compressdata pds/configdata
tgtlibs_simcam += pds/camera pds/epicstools epics/ca epics/Com
tgtslib_simcam := $(commonslib) pthread dl
tgtincs_simcam := pdsdata/include ndarray/include boost/include   


tgtsrcs_gsc16ai := gsc16ai.cc
tgtlibs_gsc16ai := $(commonlibs) pds/gsc16ai
tgtslib_gsc16ai := $(commonslib)
tgtincs_gsc16ai := pdsdata/include ndarray/include boost/include 

tgtsrcs_rayonix := rayonix.cc
tgtlibs_rayonix := $(commonlibs) pds/rayonix
tgtslib_rayonix := $(commonslib)
tgtincs_rayonix := pdsdata/include ndarray/include boost/include 

tgtsrcs_udpcam := udpcam.cc
tgtlibs_udpcam := $(commonlibs) pds/udpcam
tgtslib_udpcam := $(commonslib)
tgtincs_udpcam := pdsdata/include ndarray/include boost/include 

tgtsrcs_oceanoptics := oceanoptics.cc
tgtlibs_oceanoptics := $(commonlibs) pds/oceanoptics pds/oopt
tgtslib_oceanoptics := $(commonslib)
tgtincs_oceanoptics := pdsdata/include ndarray/include boost/include 

tgtsrcs_fli := fli.cc
tgtlibs_fli := $(commonlibs) 
tgtlibs_fli += pdsapp/configdb
tgtlibs_fli += pds/fli fli/flisdk pds/configdata
tgtslib_fli := $(commonslib) ${USRLIBDIR}/dl ${USRLIBDIR}/pthread 
tgtincs_fli := pdsdata/include ndarray/include boost/include 

tgtsrcs_andor := andor.cc
tgtlibs_andor := $(commonlibs) 
tgtlibs_andor += pdsapp/configdb
tgtlibs_andor += pds/pdsandor pds/configdata andor/andor
tgtslib_andor := $(commonslib) ${USRLIBDIR}/dl ${USRLIBDIR}/pthread 
tgtincs_andor := pdsdata/include ndarray/include boost/include 

libPicam := picam/picam picam/GenApi_gcc40_v2_2 picam/GCBase_gcc40_v2_2 picam/MathParser_gcc40_v2_2 picam/log4cpp_gcc40_v2_2 picam/Log_gcc40_v2_2
libPicam += picam/pidi picam/picc picam/pida picam/PvBase picam/PvDevice picam/PvBuffer picam/PvPersistence
libPicam += picam/PvStreamRaw picam/PvStream picam/PvGenICam picam/PvSerial picam/PtUtilsLib picam/EbNetworkLib
libPicam += picam/PtConvertersLib picam/EbTransportLayerLib picam/log4cxx

tgtsrcs_pimax := pimax.cc
tgtlibs_pimax := $(commonlibs)
tgtlibs_pimax += pdsapp/configdb
tgtlibs_pimax += pds/pdspimax pds/configdata
tgtlibs_pimax +=  $(libPicam)
tgtslib_pimax := ${USRLIBDIR}/rt ${USRLIBDIR}/dl ${USRLIBDIR}/pthread
tgtincs_pimax := pdsdata/include ndarray/include boost/include
