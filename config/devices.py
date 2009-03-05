#!/pcds/package/python-2.5.2/bin/python

from PyQt4 import QtGui,QtCore
from configdb_ui import Ui_configdb
from detinfodialog_ui_impl import Ui_detInfoDialog_impl
from CfgExpt import CfgExpt
from pds_defs import pds_typeids

class Ui_devices:
    def __init__(self,db,ui,widget):
        self.db=db
        self.ui=ui
        self.widget=widget

        widget.connect(ui.detectorList_2,
                       QtCore.SIGNAL("itemSelectionChanged()"),
                       self.update_config_list)
        widget.connect(ui.newDetectorButton,
                       QtCore.SIGNAL("clicked()"),
                       self.db_create_device)

        widget.connect(ui.configList_2,
                       QtCore.SIGNAL("itemSelectionChanged()"),
                       self.update_component_list)
        widget.connect(ui.newConfigButton_2,
                       QtCore.SIGNAL("clicked()"),
                       self.new_config)
        widget.connect(ui.copyConfigButton_2,
                       QtCore.SIGNAL("clicked()"),
                       self.copy_config)

        widget.connect(ui.componentList,
                       QtCore.SIGNAL("itemSelectionChanged()"),
                       self.change_component)
        widget.connect(ui.addComponentBox,
                       QtCore.SIGNAL("activated(const QString&)"),
                       self.add_component)

        self.update_device_list()

    def update_device_list(self):
        self.widget.disconnect(self.ui.detectorList_2,
                               QtCore.SIGNAL("itemSelectionChanged()"),
                               self.update_config_list)
        self.ui.detectorList_2.clear()
        for item in self.db.devices:
            QtGui.QListWidgetItem(item.name,self.ui.detectorList_2)
        self.widget.connect(self.ui.detectorList_2,
                            QtCore.SIGNAL("itemSelectionChanged()"),
                            self.update_config_list)

    def db_create_device(self):
        device=str(self.ui.newDetectorEdit.text())
        if not len(device):
            print "New device name \"" + device + "\" rejected"
        else:
            for item in self.db.devices:
                if item.name==device:
                    print "New device name \"" + device + "\" rejected"
                    return
            dialog=Ui_detInfoDialog_impl()
            if dialog.exec_():
                self.db.add_device(device,dialog.src_list())
                self.update_device_list()
        self.ui.newDetectorEdit.clear()

    def update_config_list(self):
        self.det=str(self.ui.detectorList_2.currentItem().text())
        self.widget.disconnect(self.ui.configList_2,
                               QtCore.SIGNAL("itemSelectionChanged"),
                               self.update_component_list)
        self.ui.configList_2.clear()
        for item in self.db.device(self.det).table.entries:
            QtGui.QListWidgetItem(item.name,
                                  self.ui.configList_2)
        self.widget.connect(self.ui.configList_2,
                            QtCore.SIGNAL("itemSelectionChanged"),
                            self.update_component_list)

    def update_component_list(self):
        self.config=str(self.ui.configList_2.currentItem().text())
        self.widget.disconnect(self.ui.componentList,
                               QtCore.SIGNAL("itemSelectionChanged()"),
                               self.change_component)
        self.ui.componentList.clear()
        self.ui.addComponentBox.clear()
        assigned=[]
        entry=self.db.device(self.det).table.get_top_entry(self.config)
        if entry:
            for item in entry.entries:
                QtGui.QListWidgetItem(item[0] + " [" + item[1] + "]",
                                      self.ui.componentList)
                assigned.append(item[0])
        for item in pds_typeids():
            if not assigned.__contains__(item):
                self.ui.addComponentBox.addItem(item)
        self.widget.connect(self.ui.componentList,
                            QtCore.SIGNAL("itemSelectionChanged()"),
                            self.change_component)
        
    def validate_config_name(self,name):
        table=[]
        for item in self.db.device(self.det).table.entries:
            table.append(item.name)
        return len(name) and not table.__contains__(name)

    def new_config(self):
        name=self.ui.newConfigEdit_2.text()
        if self.validate_config_name(name):
            self.db.device(self.det).table.new_top_entry(name)
            self.update_config_list()
        else:
            print "New config name \"" + self.det + '/' + name + "\" rejected"
        self.ui.newConfigEdit_2.clear()

    def copy_config(self):
        name=self.ui.copyConfigEdit_2.text()
        if self.validate_config_name(name):
            src=str(self.ui.configList_2.currentItem().text())
            self.db.device(self.det).table.copy_top_entry(name,src)
            self.update_config_list()
        else:
            print "New config name \"" + self.det + '/' + name + "\" rejected"
        self.ui.copyConfigEdit_2.clear()

    def change_component(self):
        type=self.ui.componentList.currentItem().text().split(" ")
        self.add_component(type)

    def add_component(self,type):
        choices=QtCore.QStringList()
        stype=str(type)
        for item in self.db.xtc_files(self.det,stype):
            choices << item
        choices << "-IMPORT-"
        [choice,ok]=QtGui.QInputDialog.getItem(self.ui.detectorList,
                                               "Component Data",
                                               "Select File",
                                               choices,0,0)
        schoice=str(choice)
        if ok:
            if schoice=="-IMPORT-":
                file=QtGui.QFileDialog.getOpenFileName(self.ui.detectorList,
                                                       "Import Data File",
                                                       "","")
                if len(file):
                    sfile=str(file)
                    self.db.import_data(self.det,stype,sfile,"")
                    schoice=os.path.basename(sfile)

            self.db.device(self.det).table.set_entry(self.config,[stype,schoice])
        self.update_component_list()
        
        
