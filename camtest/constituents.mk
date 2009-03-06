libnames := 

ifneq ($(findstring -opt,$(tgt_arch)),)

tgtnames := camdisplay
tgtnames += camtest 
tgtnames += camconfig
tgtnames += camfextest
#tgtnames += fexsimp
#tgtnames += fexnone

endif

tgtnames := camtest

commonlibs := pds/service pds/collection pds/xtc pds/utility pds/management pds/client pds/camera pds/config pdsdata/xtcdata pdsdata/opal1kdata pdsdata/camdata

tgtsrcs_camtest := camtest.cc 
tgtlibs_camtest := $(commonlibs)
tgtlibs_camtest += leutron/lvsds
tgtlibs_camtest += leutron/LvSerialCommunication.34.32
tgtlibs_camtest += leutron/LvSerialCommunication
tgtlibs_camtest += leutron/LvCamDat.34.32
tgtincs_camtest := leutron/include

tgtsrcs_camfextest := camfextest.cc 
tgtlibs_camfextest := $(commonlibs)
tgtlibs_camfextest += leutron/lvsds
tgtlibs_camfextest += leutron/LvSerialCommunication
tgtlibs_camfextest += leutron/LvCamDat.34.32
tgtincs_camfextest := leutron/include

tgtsrcs_fexsimp := fexsimp.cc 
tgtlibs_fexsimp := $(commonlibs)
tgtlibs_fexsimp += leutron/lvsds
tgtlibs_fexsimp += leutron/LvSerialCommunication
tgtlibs_fexsimp += leutron/LvCamDat.34.32
tgtincs_fexsimp := leutron/include

tgtsrcs_camdisplay := camdisplay.cc FrameDisplay.cc
tgtlibs_camdisplay := $(commonlibs)
tgtlibs_camdisplay += leutron/lvsds
tgtlibs_camdisplay += leutron/LvSerialCommunication
tgtlibs_camdisplay += leutron/LvCamDat.34.32
tgtslib_camdisplay := /pcds/package/qt-4.3.4/lib/QtGui /pcds/package/qt-4.3.4/lib/QtCore
tgtsinc_camdisplay := /pcds/package/qt-4.3.4/include

tgtsrcs_camconfig := camconfig.cc CameraFexConfig.cc

tgtsrcs_fexnone := fexnone.cc

tgtsrcs_qttest := ImageDisplayControls_moc.cc ImageDisplayFrame_moc.cc
tgtsrcs_qttest += qttest.cc ImageDisplay.cc ImageDisplayQt.cc ImageDisplayControls.cc ImageDisplayFrame.cc
tgtlibs_qttest := $(commonlibs)
tgtlibs_qttest += leutron/lvsds
tgtlibs_qttest += leutron/LvSerialCommunication
tgtlibs_qttest += leutron/LvCamDat.34.32
tgtslib_qttest := /pcds/package/qt-4.3.4/lib/QtGui /pcds/package/qt-4.3.4/lib/QtCore
tgtsinc_qttest := /pcds/package/qt-4.3.4/include
