#!/pcds/package/python-2.5.2/bin/python

from PyQt4 import QtGui,QtCore
from configdb_ui import Ui_configdb
from CfgExpt import CfgExpt

#
#  Experiment Configuration
#
class Ui_experiment:
    def __init__(self,db,ui,widget):
        self.db=db
        self.ui=ui
        self.widget=widget

        widget.connect(ui.configList,
                       QtCore.SIGNAL("itemSelectionChanged()"),
                       self.update_device_list)
        widget.connect(ui.newConfigButton,
                       QtCore.SIGNAL("clicked()"),
                       self.new_config)
        widget.connect(ui.copyConfigButton,
                       QtCore.SIGNAL("clicked()"),
                       self.copy_config)

        widget.connect(ui.detectorList,
                       QtCore.SIGNAL("itemSelectionChanged()"),
                       self.device_changed)
        widget.connect(ui.addDetectorBox,
                       QtCore.SIGNAL("activated(const QString&)"),
                       self.add_device)
        
        self.update_config_list()

    def update_config_list(self):
        self.widget.disconnect(self.ui.configList,
                               QtCore.SIGNAL("itemSelectionChanged()"),
                               self.update_device_list)
        self.ui.configList.clear()
        for item in self.db.table.entries:
            QtGui.QListWidgetItem(item.name,self.ui.configList)
        self.widget.connect(self.ui.configList,
                            QtCore.SIGNAL("itemSelectionChanged()"),
                            self.update_device_list)

    def update_device_list(self):
        self.widget.disconnect(self.ui.detectorList,
                               QtCore.SIGNAL("itemSelectionChanged()"),
                               self.device_changed)
        self.widget.disconnect(self.ui.addDetectorBox,
                               QtCore.SIGNAL("activated(const QString&)"),
                               self.add_device)
        self.ui.detectorList.clear()
        self.ui.addDetectorBox.clear()
        alias=self.ui.configList.currentItem().text()
        assigned=[]
        for item in self.db.table.get_top_entry(alias).entries:
            QtGui.QListWidgetItem(item[0] + " [" + item[1] + "]",
                                  self.ui.detectorList)
            assigned.append(item[0])
        for item in self.db.devices:
            if not assigned.__contains__(item.name):
                self.ui.addDetectorBox.addItem(item.name)
        self.widget.connect(self.ui.detectorList,
                            QtCore.SIGNAL("itemSelectionChanged()"),
                            self.device_changed)
        self.widget.connect(self.ui.addDetectorBox,
                            QtCore.SIGNAL("activated(const QString&)"),
                            self.add_device)
        
    def device_changed(self):
        device=self.ui.detectorList.currentItem().text().split(" ")[0]
        choices=QtCore.QStringList()
        for item in self.db.device(device).table.entries:
            choices << item.name
        [choice,ok]=QtGui.QInputDialog.getItem(self.ui.detectorList,
                                               device + " Configuration",
                                               "Select Config",
                                               choices,0,0)
        if ok:
            self.db.table.set_entry(self.ui.configList.currentItem().text(),
                                    [device,str(choice)])
            self.update_device_list()

    def validate_config_name(self,name):
        list=self.db.table.get_top_names()
        return len(name) and not list.__contains__(name)
        
    def new_config(self):
        name=self.ui.newConfigEdit.text()
        if self.validate_config_name(name):
            self.db.table.new_top_entry(name)
            self.update_config_list()
        else:
            print "New config name \"" + name + "\" rejected"
        self.ui.newConfigEdit.clear()

    def copy_config(self):
        name=self.ui.copyConfigEdit.text()
        if self.validate_config_name(name):
            src=self.ui.configList.currentItem().text()
            self.db.table.copy_top_entry(name,src)
            self.update_config_list()
        else:
            print "New config name \"" + name + "\" rejected"
        self.ui.copyConfigEdit.clear()

    def add_device(self,name):
        device=str(name)
        choices=QtCore.QStringList()
        for item in self.db.device(device).table.entries:
            choices << item.name
        [choice,ok]=QtGui.QInputDialog.getItem(self.ui.detectorList,
                                               device + " Configuration",
                                               "Select Config",
                                               choices,0,0)
        if ok:
            self.db.table.set_entry(self.ui.configList.currentItem().text(),
                                    [device,str(choice)])
            self.update_device_list()
        
        
    

