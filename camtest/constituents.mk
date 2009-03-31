libnames := 

tgtnames := camanalysis

commonlibs := pdsdata/xtcdata pdsdata/opal1kdata pdsdata/camdata
commonlibs += pds/service pds/collection pds/xtc 
commonlibs += pds/mon pds/vmon
commonlibs += pds/utility pds/management pds/client pds/camera pds/config 

tgtsrcs_camanalysis := camanalysis.cc
tgtlibs_camanalysis := $(commonlibs) pds/mon
tgtlibs_camanalysis += leutron/lvsds
tgtlibs_camanalysis += leutron/LvSerialCommunication.34.32
tgtlibs_camanalysis += leutron/LvSerialCommunication
tgtlibs_camanalysis += leutron/LvCamDat.34.32
tgtslib_camanalysis := /usr/lib/rt
