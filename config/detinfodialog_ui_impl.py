#!/pcds/package/python-2.5.2/bin/python

from PyQt4 import QtGui,QtCore
from detinfodialog_ui import Ui_detInfoDialog
from pds_defs import pds_detectors,pds_devices

class Ui_detInfoDialog_impl(QtGui.QDialog):
    def __init__(self):
        QtGui.QDialog.__init__(self)

        self.ui=Ui_detInfoDialog()
        self.ui.setupUi(self)

        self.srclist=[]
        
        for item in pds_detectors():
            self.ui.detInfoDetBox.addItem(item)
        edit=self.ui.detInfoDetIdEdit
        edit.setValidator(QtGui.QIntValidator(0,0xff,edit))

        for item in pds_devices():
            self.ui.detInfoDevBox.addItem(item)
        edit=self.ui.detInfoDevIdEdit
        edit.setValidator(QtGui.QIntValidator(0,0xff,edit))

        self.connect(self.ui.addButton, QtCore.SIGNAL("clicked()"), self.add_src)
        self.connect(self.ui.removeButton, QtCore.SIGNAL("clicked()"), self.remove_src)

    def add_src(self):
        dettype=self.ui.detInfoDetBox.currentIndex()
        devtype=self.ui.detInfoDevBox.currentIndex()
        [detid,detok]=self.ui.detInfoDetIdEdit.text().toInt()
        [devid,devok]=self.ui.detInfoDevIdEdit.text().toInt()
        detinfo=(((dettype&0xff)<<24) |
                 ((detid&0xff)<<16) |
                 ((devtype&0xff)<<8) |
                 ((devid & 0xff)<<0))
        item="%08x" % (detinfo)
        self.srclist.append(item)
        self.ui.listWidget.addItem(item)
        
    def remove_src(self):
        idx=self.ui.listWidget.currentRow()
        list=self.srclist
        self.srclist=list[0:idx]+list[idx+1:]
        self.ui.listWidget.takeItem(idx)

    def src_list(self):
        return self.srclist

