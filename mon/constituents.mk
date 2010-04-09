ifneq ($(findstring x86_64-linux,$(tgt_arch)),)
syslibdir := /usr/lib64
qtincdir  := qt/include64
else
syslibdir := /usr/lib
qtincdir  := qt/include
endif

libnames := monapp

libsrcs_monapp := $(filter-out MonMain.cc MonTreeMenu.cc MonTreeMenu_moc.cc, $(wildcard Mon*.cc))
libsrcs_monapp += MonDialog_moc.cc MonQtImageDisplay_moc.cc MonTab_moc.cc MonCanvas_moc.cc MonTree_moc.cc
libincs_monapp := $(qtincdir)
libincs_monapp += qwt/include
# qwt includes qt headers without package prefix!
libincs_monapp += $(qtincdir)/Qt
libincs_monapp += $(qtincdir)/Qt3Support
libincs_monapp += $(qtincdir)/QtAssistant
libincs_monapp += $(qtincdir)/QtCore
libincs_monapp += $(qtincdir)/QtDesigner
libincs_monapp += $(qtincdir)/QtGui
libincs_monapp += $(qtincdir)/QtNetwork
libincs_monapp += $(qtincdir)/QtOpenGL
libincs_monapp += $(qtincdir)/QtScript
libincs_monapp += $(qtincdir)/QtSql
libincs_monapp += $(qtincdir)/QtSvg
libincs_monapp += $(qtincdir)/QtTest
libincs_monapp += $(qtincdir)/QtUiTools
libincs_monapp += $(qtincdir)/QtXml


tgtnames := vmondisplay mondisplay monservertest vmonservertest vmonreader


tgtsrcs_vmondisplay += VmonTreeMenu.cc VmonTreeMenu_moc.cc
tgtsrcs_vmondisplay += VmonMain.cc
tgtsrcs_vmondisplay += vmondisplay.cc
tgtlibs_vmondisplay := pdsdata/xtcdata pds/service pds/collection pds/xtc pds/mon pds/vmon pds/utility pds/management pdsapp/monapp
tgtlibs_vmondisplay += qt/QtGui qt/QtCore
tgtlibs_vmondisplay += qwt/qwt
tgtincs_vmondisplay := $(qtincdir)
tgtincs_vmondisplay += qwt/include

tgtsrcs_mondisplay += MonTreeMenu.cc MonTreeMenu_moc.cc
tgtsrcs_mondisplay += MonMain.cc
tgtsrcs_mondisplay += mondisplay.cc
tgtlibs_mondisplay := pdsdata/xtcdata pds/service pds/mon pdsapp/monapp
tgtlibs_mondisplay += qt/QtGui qt/QtCore
tgtlibs_mondisplay += qwt/qwt
tgtincs_mondisplay := $(qtincdir)
tgtincs_mondisplay += qwt/include

tgtsrcs_monservertest += monservertest.cc
tgtlibs_monservertest := pdsdata/xtcdata pds/service pds/mon
tgtslib_monservertest := $(syslibdir)/rt

tgtsrcs_vmonservertest += vmonservertest.cc
tgtlibs_vmonservertest := pdsdata/xtcdata pds/service pds/xtc pds/collection pds/mon pds/vmon pds/utility pds/management
tgtslib_vmonservertest := $(syslibdir)/rt

tgtsrcs_vmonreader += VmonReaderDump.cc
tgtlibs_vmonreader := pdsdata/xtcdata pds/service pds/xtc pds/collection pds/mon pds/vmon pds/utility pds/management
tgtslib_vmonreader := $(syslibdir)/rt


