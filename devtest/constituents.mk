CPPFLAGS += -D_ACQIRIS -D_LINUX

tgtnames    := evr evgr acq acqevr opal1k

tgtsrcs_acq := acq.cc
tgtincs_acq := acqiris
tgtlibs_acq := pdsdata/xtcdata pdsdata/acqdata acqiris/AqDrv4 pds/service pds/collection pds/xtc pds/mon pds/vmon pds/utility pds/acqiris pds/management pds/client 
tgtslib_acq := /usr/lib/rt

tgtsrcs_acqevr := acqevr.cc
tgtincs_acqevr := acqiris
tgtlibs_acqevr := pdsdata/xtcdata pdsdata/evrdata evgr/evr evgr/evg pdsdata/acqdata acqiris/AqDrv4 pds/service pds/collection pds/config pds/xtc pds/mon pds/vmon pds/utility pds/acqiris pds/evgr pds/management pds/client
tgtslib_acqevr := /usr/lib/rt

tgtsrcs_evr := evr.cc
tgtincs_evr := evgr
tgtlibs_evr := pdsdata/xtcdata pdsdata/evrdata
tgtlibs_evr += evgr/evr evgr/evg 
tgtlibs_evr += pds/service pds/collection pds/config pds/xtc pds/mon pds/vmon pds/utility pds/management pds/client pds/evgr 
tgtslib_evr := /usr/lib/rt

tgtsrcs_evrobs := evrobs.cc
tgtincs_evrobs := evgr
tgtlibs_evrobs := pdsdata/xtcdata evgr/evr evgr/evg pds/service pds/collection pds/xtc pds/utility pds/management pds/client pds/evgr 
tgtslib_evrobs := /usr/lib/rt

tgtsrcs_evgr := evgr.cc
tgtincs_evgr := evgr
tgtlibs_evgr := pdsdata/xtcdata pdsdata/evrdata evgr/evg evgr/evr pds/service pds/collection pds/config pds/xtc pds/mon pds/vmon pds/utility pds/management pds/client pds/evgr 
tgtslib_evgr := /usr/lib/rt

tgtsrcs_opal1k := opal1k.cc 
tgtlibs_opal1k := pdsdata/xtcdata pdsdata/opal1kdata pdsdata/camdata
tgtlibs_opal1k += pds/service pds/collection pds/xtc pds/mon pds/vmon pds/utility pds/management pds/client pds/camera pds/config
tgtlibs_opal1k += leutron/lvsds.34.32
tgtlibs_opal1k += leutron/LvSerialCommunication.34.32
tgtlibs_opal1k += leutron/LvCamDat.34.32
tgtincs_opal1k := leutron/include
