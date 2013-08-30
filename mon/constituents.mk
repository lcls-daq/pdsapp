libnames := monapp

libsrcs_monapp := $(filter-out MonMain.cc MonTreeMenu.cc MonTreeMenu_moc.cc, $(wildcard Mon*.cc))
libsrcs_monapp += MonDialog_moc.cc MonQtImageDisplay_moc.cc MonTab_moc.cc MonCanvas_moc.cc MonTree_moc.cc
libincs_monapp := $(qtincdir)
libincs_monapp += $(qwtincs) qwt/include
libincs_monapp += pdsdata/include
libsinc_monapp += $(qwtsinc)

tgtnames := vmondisplay mondisplay monservertest vmonservertest vmonreader vmonrecorder


tgtsrcs_vmondisplay += VmonTreeMenu.cc VmonTreeMenu_moc.cc
tgtsrcs_vmondisplay += VmonMain.cc
tgtsrcs_vmondisplay += vmondisplay.cc
tgtlibs_vmondisplay := pdsdata/xtcdata pdsdata/aliasdata pds/service pds/collection pds/xtc pds/mon pds/vmon pds/utility pds/management pdsapp/monapp
tgtlibs_vmondisplay += $(qtlibdir)
tgtlibs_vmondisplay += qwt/qwt
tgtincs_vmondisplay := $(qtincdir)
tgtincs_vmondisplay += $(qwtincs) qwt/include
tgtincs_vmondisplay += pdsdata/include

tgtsrcs_mondisplay += MonTreeMenu.cc MonTreeMenu_moc.cc
tgtsrcs_mondisplay += MonMain.cc
tgtsrcs_mondisplay += mondisplay.cc
tgtlibs_mondisplay := pdsdata/xtcdata pdsdata/aliasdata pds/service pds/mon pdsapp/monapp
tgtlibs_mondisplay += $(qtlibdir)
tgtlibs_mondisplay += qwt/qwt
tgtincs_mondisplay := $(qtincdir)
tgtincs_mondisplay += $(qwtincs) qwt/include
tgtincs_mondisplay += pdsdata/include

tgtsrcs_monservertest += monservertest.cc
tgtlibs_monservertest := pdsdata/xtcdata pdsdata/aliasdata pds/service pds/mon
tgtslib_monservertest := $(USRLIBDIR)/rt
tgtincs_monservertest := pdsdata/include

tgtsrcs_vmonservertest += vmonservertest.cc
tgtlibs_vmonservertest := pdsdata/xtcdata pdsdata/aliasdata pds/service pds/xtc pds/collection pds/mon pds/vmon pds/utility pds/management
tgtslib_vmonservertest := $(USRLIBDIR)/rt
tgtincs_vmonservertest := pdsdata/include

tgtsrcs_vmonreader += VmonReaderDump.cc
tgtlibs_vmonreader := pdsdata/xtcdata pdsdata/aliasdata pds/service pds/xtc pds/collection pds/mon pds/vmon pds/utility pds/management
tgtslib_vmonreader := $(USRLIBDIR)/rt
tgtincs_vmonreader := pdsdata/include

tgtsrcs_vmonrecorder += vmonrecorder.cc
tgtlibs_vmonrecorder := pdsdata/xtcdata pdsdata/aliasdata pds/service pds/xtc pds/collection pds/mon pds/vmon pds/utility pds/management
tgtslib_vmonrecorder := $(USRLIBDIR)/rt
tgtincs_vmonrecorder := pdsdata/include

