libnames := 

ifneq ($(findstring -opt,$(tgt_arch)),)

tgtnames := camdisplay
tgtnames += camtest 
tgtnames += camconfig
tgtnames += camfextest
#tgtnames += fexsimp
#tgtnames += fexnone

endif

tgtsrcs_camtest := camtest.cc 
tgtsrcs_camtest += FexFrameServer.cc 
tgtsrcs_camtest += CameraFexConfig.cc
tgtsrcs_camtest += Opal1kConfig.cc
tgtsrcs_camtest += TwoDGaussian.cc 
tgtlibs_camtest := pds/diagnostic pds/service pds/collection pds/xtc pds/utility pds/management pds/client pds/camera pds/config pdsdata/xtcdata
tgtlibs_camtest += leutron/lvsds
tgtlibs_camtest += leutron/LvSerialCommunication
tgtlibs_camtest += leutron/LvCamDat.34.32
tgtincs_camtest := leutron/include

tgtsrcs_camfextest := camfextest.cc 
tgtsrcs_camfextest += Opal1kConfig.cc
tgtsrcs_camfextest += TwoDGaussian.cc 
tgtlibs_camfextest := pds/diagnostic pds/service pds/collection pds/xtc pds/utility pds/management pds/client pds/camera pds/config pdsdata/xtcdata
tgtlibs_camfextest += leutron/lvsds
tgtlibs_camfextest += leutron/LvSerialCommunication
tgtlibs_camfextest += leutron/LvCamDat.34.32
tgtincs_camfextest := leutron/include

tgtsrcs_fexsimp := fexsimp.cc 
tgtlibs_fexsimp := pds/diagnostic pds/service pds/collection pds/xtc pds/utility pds/management pds/client pds/camera pds/config pdsdata/xtcdata
tgtlibs_fexsimp += leutron/lvsds
tgtlibs_fexsimp += leutron/LvSerialCommunication
tgtlibs_fexsimp += leutron/LvCamDat.34.32
tgtincs_fexsimp := leutron/include

tgtsrcs_camdisplay := camdisplay.cc FrameDisplay.cc
tgtlibs_camdisplay := pds/service pds/collection pds/xtc pds/utility pds/management pds/client pdsdata/xtcdata pds/diagnostic
tgtslib_camdisplay := /pcds/package/qt-4.3.4/lib/QtGui /pcds/package/qt-4.3.4/lib/QtCore
tgtsinc_camdisplay := /pcds/package/qt-4.3.4/include

tgtsrcs_camconfig := camconfig.cc CameraFexConfig.cc

tgtsrcs_fexnone := fexnone.cc
