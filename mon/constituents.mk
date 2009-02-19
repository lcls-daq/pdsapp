libnames := 

tgtnames := mondisplay monservertest


tgtsrcs_mondisplay := MonPath.cc
tgtsrcs_mondisplay += MonUtils.cc
tgtsrcs_mondisplay += MonRasterData.cc
tgtsrcs_mondisplay += MonQtEntry.cc
tgtsrcs_mondisplay += MonQtBase.cc
tgtsrcs_mondisplay += MonQtChart.cc
tgtsrcs_mondisplay += MonQtTH1F.cc
tgtsrcs_mondisplay += MonQtTH2F.cc
tgtsrcs_mondisplay += MonQtProf.cc
tgtsrcs_mondisplay += MonQtImage.cc
tgtsrcs_mondisplay += MonQtImageDisplay.cc MonQtImageDisplay_moc.cc
tgtsrcs_mondisplay += MonCanvas.cc MonCanvas_moc.cc
tgtsrcs_mondisplay += MonDialog.cc MonDialog_moc.cc
tgtsrcs_mondisplay += MonConsumerTH1F.cc
tgtsrcs_mondisplay += MonConsumerTH2F.cc
tgtsrcs_mondisplay += MonConsumerProf.cc
tgtsrcs_mondisplay += MonConsumerImage.cc
tgtsrcs_mondisplay += MonConsumerFactory.cc
tgtsrcs_mondisplay += MonTab.cc MonTab_moc.cc
tgtsrcs_mondisplay += MonTabMenu.cc
tgtsrcs_mondisplay += MonTree.cc MonTree_moc.cc
tgtsrcs_mondisplay += MonTreeMenu.cc MonTreeMenu_moc.cc
tgtsrcs_mondisplay += MonMain.cc
tgtsrcs_mondisplay += mondisplay.cc

tgtlibs_mondisplay := pds/service pds/collection pds/xtc pds/utility pds/management pds/client pds/config pdsdata/xtcdata pds/mon
tgtlibs_mondisplay += qt/QtGui qt/QtCore
tgtlibs_mondisplay += qwt/qwt

tgtsinc_mondisplay := /pcds/package/qt-4.3.4/include
tgtsinc_mondisplay += /usr/local/qwt-5.1.1/include

# qwt includes qt headers without package prefix!
tgtsinc_mondisplay += /pcds/package/qt-4.3.4/include/Qt
tgtsinc_mondisplay += /pcds/package/qt-4.3.4/include/Qt3Support
tgtsinc_mondisplay += /pcds/package/qt-4.3.4/include/QtAssistant
tgtsinc_mondisplay += /pcds/package/qt-4.3.4/include/QtCore
tgtsinc_mondisplay += /pcds/package/qt-4.3.4/include/QtDesigner
tgtsinc_mondisplay += /pcds/package/qt-4.3.4/include/QtGui
tgtsinc_mondisplay += /pcds/package/qt-4.3.4/include/QtNetwork
tgtsinc_mondisplay += /pcds/package/qt-4.3.4/include/QtOpenGL
tgtsinc_mondisplay += /pcds/package/qt-4.3.4/include/QtScript
tgtsinc_mondisplay += /pcds/package/qt-4.3.4/include/QtSql
tgtsinc_mondisplay += /pcds/package/qt-4.3.4/include/QtSvg
tgtsinc_mondisplay += /pcds/package/qt-4.3.4/include/QtTest
tgtsinc_mondisplay += /pcds/package/qt-4.3.4/include/QtUiTools
tgtsinc_mondisplay += /pcds/package/qt-4.3.4/include/QtXml

tgtsrcs_monservertest += monservertest.cc
tgtlibs_monservertest := pds/service pds/collection pds/xtc pds/utility pds/management pds/client pds/config pdsdata/xtcdata pds/mon
tgtslib_monservertest := /usr/lib/rt
