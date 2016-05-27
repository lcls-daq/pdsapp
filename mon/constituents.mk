libnames := monapp

libsrcs_monapp := $(filter-out MonMain.cc MonTreeMenu.cc MonTreeMenu_moc.cc MonTabMenu.cc, $(wildcard Mon*.cc))
libsrcs_monapp += MonDialog_moc.cc MonQtImageDisplay_moc.cc MonTab_moc.cc MonCanvas_moc.cc MonTree_moc.cc
libsrcs_monapp += QtPlotCurve.cc QtPlotCurve_moc.cc
libincs_monapp := $(qtincdir)
libincs_monapp += $(qwtincs) qwt/include
libincs_monapp += pdsdata/include
libsinc_monapp += $(qwtsinc)

tgtnames := vmondisplay vmonservertest vmonreader vmonrecorder


tgtsrcs_vmondisplay += VmonTreeMenu.cc VmonTreeMenu_moc.cc 
tgtsrcs_vmondisplay += VmonMain.cc
tgtsrcs_vmondisplay += vmondisplay.cc
tgtlibs_vmondisplay := pdsdata/xtcdata pdsdata/psddl_pdsdata pds/service pds/collection pds/xtc pds/mon pds/vmon pds/utility pds/management 
tgtlibs_vmondisplay += pdsapp/monapp
tgtlibs_vmondisplay += $(qtlibdir)
tgtlibs_vmondisplay += qwt/qwt
tgtincs_vmondisplay := $(qtincdir)
tgtincs_vmondisplay += $(qwtincs) qwt/include
tgtincs_vmondisplay += pdsdata/include

tgtsrcs_vmonservertest += vmonservertest.cc
tgtlibs_vmonservertest := pdsdata/xtcdata pdsdata/psddl_pdsdata pds/service pds/xtc pds/collection pds/mon pds/vmon pds/utility pds/management
tgtslib_vmonservertest := $(USRLIBDIR)/rt
tgtincs_vmonservertest := pdsdata/include

tgtsrcs_vmonreader += vmonreader.cc VmonReaderTreeMenu.cc VmonReaderTreeMenu_moc.cc 
tgtlibs_vmonreader := pdsdata/xtcdata pdsdata/psddl_pdsdata pds/service pds/xtc pds/collection pds/mon pds/vmon pds/utility pds/management 
tgtlibs_vmonreader += pdsapp/monapp
tgtlibs_vmonreader += $(qtlibdir)  qwt/qwt
tgtslib_vmonreader := $(USRLIBDIR)/rt
tgtincs_vmonreader := pdsdata/include $(qtincdir)

tgtsrcs_vmonrecorder += vmonrecorder.cc
tgtlibs_vmonrecorder := pdsdata/xtcdata pdsdata/psddl_pdsdata pds/service pds/xtc pds/collection pds/mon pds/vmon pds/utility pds/management 
tgtslib_vmonrecorder := $(USRLIBDIR)/rt
tgtincs_vmonrecorder := pdsdata/include

