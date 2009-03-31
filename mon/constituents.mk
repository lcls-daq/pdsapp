libnames := monapp

libsrcs_monapp := $(filter-out MonMain.cc MonTreeMenu.cc MonTreeMenu_moc.cc, $(wildcard Mon*.cc))
libsinc_monapp := /pcds/package/qt-4.3.4/include
libsinc_monapp += /pcds/package/external/qwt-5.1.1/include
# qwt includes qt headers without package prefix!
libsinc_monapp += /pcds/package/qt-4.3.4/include/Qt
libsinc_monapp += /pcds/package/qt-4.3.4/include/Qt3Support
libsinc_monapp += /pcds/package/qt-4.3.4/include/QtAssistant
libsinc_monapp += /pcds/package/qt-4.3.4/include/QtCore
libsinc_monapp += /pcds/package/qt-4.3.4/include/QtDesigner
libsinc_monapp += /pcds/package/qt-4.3.4/include/QtGui
libsinc_monapp += /pcds/package/qt-4.3.4/include/QtNetwork
libsinc_monapp += /pcds/package/qt-4.3.4/include/QtOpenGL
libsinc_monapp += /pcds/package/qt-4.3.4/include/QtScript
libsinc_monapp += /pcds/package/qt-4.3.4/include/QtSql
libsinc_monapp += /pcds/package/qt-4.3.4/include/QtSvg
libsinc_monapp += /pcds/package/qt-4.3.4/include/QtTest
libsinc_monapp += /pcds/package/qt-4.3.4/include/QtUiTools
libsinc_monapp += /pcds/package/qt-4.3.4/include/QtXml


tgtnames := vmondisplay mondisplay monservertest vmonservertest


tgtsrcs_vmondisplay += VmonTreeMenu.cc VmonTreeMenu_moc.cc
tgtsrcs_vmondisplay += VmonMain.cc
tgtsrcs_vmondisplay += vmondisplay.cc
tgtlibs_vmondisplay := pdsdata/xtcdata pds/service pds/collection pds/xtc pds/mon pds/vmon pds/utility pds/management pdsapp/monapp
tgtlibs_vmondisplay += qt/QtGui qt/QtCore
tgtlibs_vmondisplay += qwt/qwt
tgtsinc_vmondisplay := /pcds/package/qt-4.3.4/include
tgtsinc_vmondisplay += /pcds/package/external/qwt-5.1.1/include

tgtsrcs_mondisplay += MonTreeMenu.cc MonTreeMenu_moc.cc
tgtsrcs_mondisplay += MonMain.cc
tgtsrcs_mondisplay += mondisplay.cc
tgtlibs_mondisplay := pdsdata/xtcdata pds/service pds/mon pdsapp/monapp
tgtlibs_mondisplay += qt/QtGui qt/QtCore
tgtlibs_mondisplay += qwt/qwt
tgtsinc_mondisplay := /pcds/package/qt-4.3.4/include
tgtsinc_mondisplay += /pcds/package/external/qwt-5.1.1/include

tgtsrcs_monservertest += monservertest.cc
tgtlibs_monservertest := pdsdata/xtcdata pds/service pds/mon
tgtslib_monservertest := /usr/lib/rt

tgtsrcs_vmonservertest += vmonservertest.cc
tgtlibs_vmonservertest := pdsdata/xtcdata pds/service pds/xtc pds/collection pds/mon pds/vmon pds/utility pds/management
tgtslib_vmonservertest := /usr/lib/rt


