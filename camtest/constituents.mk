libnames := 

tgtnames := camtest cammonitor camanalysis

commonlibs := pds/service pds/collection pds/xtc pds/utility pds/management pds/client pds/camera pds/config pdsdata/xtcdata pdsdata/opal1kdata pdsdata/camdata

tgtsrcs_camtest := camtest.cc 
tgtlibs_camtest := $(commonlibs)
tgtlibs_camtest += leutron/lvsds
tgtlibs_camtest += leutron/LvSerialCommunication.34.32
tgtlibs_camtest += leutron/LvSerialCommunication
tgtlibs_camtest += leutron/LvCamDat.34.32
tgtincs_camtest := leutron/include

tgtsrcs_cammonitor := cammon.cc CamDisplay.cc
tgtlibs_cammonitor := $(commonlibs) pds/mon
tgtlibs_cammonitor += leutron/lvsds
tgtlibs_cammonitor += leutron/LvSerialCommunication.34.32
tgtlibs_cammonitor += leutron/LvSerialCommunication
tgtlibs_cammonitor += leutron/LvCamDat.34.32
tgtslib_cammonitor := /usr/lib/rt

tgtsrcs_camanalysis := camanalysis.cc
tgtlibs_camanalysis := $(commonlibs) pds/mon
tgtlibs_camanalysis += leutron/lvsds
tgtlibs_camanalysis += leutron/LvSerialCommunication.34.32
tgtlibs_camanalysis += leutron/LvSerialCommunication
tgtlibs_camanalysis += leutron/LvCamDat.34.32
tgtslib_camanalysis := /usr/lib/rt
