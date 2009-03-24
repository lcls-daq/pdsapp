libnames := 

ifneq ($(findstring -opt,$(tgt_arch)),)
tgtnames := camtest camanalysis
else
tgtnames :=
endif

commonlibs := pds/service pds/collection pds/xtc pds/utility pds/management pds/client pds/camera pds/config pdsdata/xtcdata pdsdata/opal1kdata pdsdata/camdata

tgtsrcs_camtest := camtest.cc 
tgtlibs_camtest := $(commonlibs)
tgtlibs_camtest += leutron/lvsds
tgtlibs_camtest += leutron/LvSerialCommunication.34.32
tgtlibs_camtest += leutron/LvSerialCommunication
tgtlibs_camtest += leutron/LvCamDat.34.32
tgtincs_camtest := leutron/include

tgtsrcs_camanalysis := camanalysis.cc
tgtlibs_camanalysis := $(commonlibs) pds/mon
tgtlibs_camanalysis += leutron/lvsds
tgtlibs_camanalysis += leutron/LvSerialCommunication.34.32
tgtlibs_camanalysis += leutron/LvSerialCommunication
tgtlibs_camanalysis += leutron/LvCamDat.34.32
tgtslib_camanalysis := /usr/lib/rt
