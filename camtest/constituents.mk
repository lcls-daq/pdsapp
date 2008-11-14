libnames := 
 
tgtnames          := camtest

tgtsrcs_camtest := camtest.cc FrameServer.cc
tgtlibs_camtest := pdsdata/xtcdata pds/service pds/collection pds/xtc pds/utility pds/management pds/client pds/camera
tgtlibs_camtest += leutron/lvsds
tgtlibs_camtest += leutron/LvSerialCommunication
tgtincs_camtest := leutron/include
