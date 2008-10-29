#
#  Only build for i386-linux.  (rdcds211)
#
ifneq ($(findstring i386-linux-,$(tgt_arch)),)

libnames := 
 
#libsrcs_test := EventTest.cc EventOptions.cc

tgtnames          := camtest

tgtsrcs_camtest := camtest.cc FrameServer.cc
tgtlibs_camtest := pds/service pds/collection pds/xtc pds/utility pds/management pds/client pds/camera
tgtslib_camtest := /usr/lib/lvsds
tgtsinc_camtest := /usr/include/lvsds

endif
