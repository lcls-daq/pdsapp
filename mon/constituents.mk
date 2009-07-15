libnames := monapp

libsrcs_monapp := $(filter-out MonMain.cc MonTreeMenu.cc MonTreeMenu_moc.cc, $(wildcard Mon*.cc))
libsrcs_monapp += MonDialog_moc.cc MonQtImageDisplay_moc.cc MonTab_moc.cc MonCanvas_moc.cc MonTree_moc.cc
libincs_monapp := qt/include
libincs_monapp += qwt/include
# qwt includes qt headers without package prefix!
libincs_monapp += qt/include/Qt
libincs_monapp += qt/include/Qt3Support
libincs_monapp += qt/include/QtAssistant
libincs_monapp += qt/include/QtCore
libincs_monapp += qt/include/QtDesigner
libincs_monapp += qt/include/QtGui
libincs_monapp += qt/include/QtNetwork
libincs_monapp += qt/include/QtOpenGL
libincs_monapp += qt/include/QtScript
libincs_monapp += qt/include/QtSql
libincs_monapp += qt/include/QtSvg
libincs_monapp += qt/include/QtTest
libincs_monapp += qt/include/QtUiTools
libincs_monapp += qt/include/QtXml


tgtnames := vmondisplay mondisplay monservertest vmonservertest vmonreader


tgtsrcs_vmondisplay += VmonTreeMenu.cc VmonTreeMenu_moc.cc
tgtsrcs_vmondisplay += VmonMain.cc
tgtsrcs_vmondisplay += vmondisplay.cc
tgtlibs_vmondisplay := pdsdata/xtcdata pds/service pds/collection pds/xtc pds/mon pds/vmon pds/utility pds/management pdsapp/monapp
tgtlibs_vmondisplay += qt/QtGui qt/QtCore
tgtlibs_vmondisplay += qwt/qwt
tgtincs_vmondisplay := qt/include
tgtincs_vmondisplay += qwt/include

tgtsrcs_mondisplay += MonTreeMenu.cc MonTreeMenu_moc.cc
tgtsrcs_mondisplay += MonMain.cc
tgtsrcs_mondisplay += mondisplay.cc
tgtlibs_mondisplay := pdsdata/xtcdata pds/service pds/mon pdsapp/monapp
tgtlibs_mondisplay += qt/QtGui qt/QtCore
tgtlibs_mondisplay += qwt/qwt
tgtincs_mondisplay := qt/include
tgtincs_mondisplay += qwt/include

tgtsrcs_monservertest += monservertest.cc
tgtlibs_monservertest := pdsdata/xtcdata pds/service pds/mon
tgtslib_monservertest := /usr/lib/rt

tgtsrcs_vmonservertest += vmonservertest.cc
tgtlibs_vmonservertest := pdsdata/xtcdata pds/service pds/xtc pds/collection pds/mon pds/vmon pds/utility pds/management
tgtslib_vmonservertest := /usr/lib/rt

tgtsrcs_vmonreader += VmonReaderDump.cc
tgtlibs_vmonreader := pdsdata/xtcdata pds/service pds/xtc pds/collection pds/mon pds/vmon pds/utility pds/management
tgtslib_vmonreader := /usr/lib/rt


