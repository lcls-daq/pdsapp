# -*- coding: utf-8 -*-

# Form implementation generated from reading ui file 'pdsapp/config/configdb.ui'
#
# Created: Wed Feb 25 09:01:44 2009
#      by: PyQt4 UI code generator 4.3.3
#
# WARNING! All changes made in this file will be lost!

from PyQt4 import QtCore, QtGui

class Ui_configdb(object):
    def setupUi(self, configdb):
        configdb.setObjectName("configdb")
        configdb.resize(QtCore.QSize(QtCore.QRect(0,0,968,842).size()).expandedTo(configdb.minimumSizeHint()))

        self.detectorConfigBox = QtGui.QGroupBox(configdb)
        self.detectorConfigBox.setGeometry(QtCore.QRect(10,450,808,327))
        self.detectorConfigBox.setObjectName("detectorConfigBox")

        self.hboxlayout = QtGui.QHBoxLayout(self.detectorConfigBox)
        self.hboxlayout.setObjectName("hboxlayout")

        self.vboxlayout = QtGui.QVBoxLayout()
        self.vboxlayout.setObjectName("vboxlayout")

        self.detectorLabel_2 = QtGui.QLabel(self.detectorConfigBox)
        self.detectorLabel_2.setAlignment(QtCore.Qt.AlignCenter)
        self.detectorLabel_2.setObjectName("detectorLabel_2")
        self.vboxlayout.addWidget(self.detectorLabel_2)

        self.detectorList_2 = QtGui.QListWidget(self.detectorConfigBox)
        self.detectorList_2.setObjectName("detectorList_2")
        self.vboxlayout.addWidget(self.detectorList_2)

        self.hboxlayout1 = QtGui.QHBoxLayout()
        self.hboxlayout1.setObjectName("hboxlayout1")

        self.newDetectorEdit = QtGui.QLineEdit(self.detectorConfigBox)
        self.newDetectorEdit.setObjectName("newDetectorEdit")
        self.hboxlayout1.addWidget(self.newDetectorEdit)

        self.newDetectorButton = QtGui.QPushButton(self.detectorConfigBox)
        self.newDetectorButton.setObjectName("newDetectorButton")
        self.hboxlayout1.addWidget(self.newDetectorButton)
        self.vboxlayout.addLayout(self.hboxlayout1)
        self.hboxlayout.addLayout(self.vboxlayout)

        self.vboxlayout1 = QtGui.QVBoxLayout()
        self.vboxlayout1.setObjectName("vboxlayout1")

        self.configLabel = QtGui.QLabel(self.detectorConfigBox)
        self.configLabel.setAlignment(QtCore.Qt.AlignCenter)
        self.configLabel.setObjectName("configLabel")
        self.vboxlayout1.addWidget(self.configLabel)

        self.configList_2 = QtGui.QListWidget(self.detectorConfigBox)
        self.configList_2.setObjectName("configList_2")
        self.vboxlayout1.addWidget(self.configList_2)

        self.hboxlayout2 = QtGui.QHBoxLayout()
        self.hboxlayout2.setObjectName("hboxlayout2")

        self.newConfigEdit_2 = QtGui.QLineEdit(self.detectorConfigBox)
        self.newConfigEdit_2.setObjectName("newConfigEdit_2")
        self.hboxlayout2.addWidget(self.newConfigEdit_2)

        self.newConfigButton_2 = QtGui.QPushButton(self.detectorConfigBox)
        self.newConfigButton_2.setObjectName("newConfigButton_2")
        self.hboxlayout2.addWidget(self.newConfigButton_2)
        self.vboxlayout1.addLayout(self.hboxlayout2)

        self.hboxlayout3 = QtGui.QHBoxLayout()
        self.hboxlayout3.setObjectName("hboxlayout3")

        self.copyConfigEdit_2 = QtGui.QLineEdit(self.detectorConfigBox)
        self.copyConfigEdit_2.setObjectName("copyConfigEdit_2")
        self.hboxlayout3.addWidget(self.copyConfigEdit_2)

        self.copyConfigButton_2 = QtGui.QPushButton(self.detectorConfigBox)
        self.copyConfigButton_2.setObjectName("copyConfigButton_2")
        self.hboxlayout3.addWidget(self.copyConfigButton_2)
        self.vboxlayout1.addLayout(self.hboxlayout3)
        self.hboxlayout.addLayout(self.vboxlayout1)

        self.vboxlayout2 = QtGui.QVBoxLayout()
        self.vboxlayout2.setObjectName("vboxlayout2")

        self.componentLabel = QtGui.QLabel(self.detectorConfigBox)
        self.componentLabel.setAlignment(QtCore.Qt.AlignCenter)
        self.componentLabel.setObjectName("componentLabel")
        self.vboxlayout2.addWidget(self.componentLabel)

        self.componentList = QtGui.QListWidget(self.detectorConfigBox)
        self.componentList.setObjectName("componentList")
        self.vboxlayout2.addWidget(self.componentList)

        self.hboxlayout4 = QtGui.QHBoxLayout()
        self.hboxlayout4.setObjectName("hboxlayout4")

        self.addComponentLabel = QtGui.QLabel(self.detectorConfigBox)
        self.addComponentLabel.setAlignment(QtCore.Qt.AlignRight|QtCore.Qt.AlignTrailing|QtCore.Qt.AlignVCenter)
        self.addComponentLabel.setObjectName("addComponentLabel")
        self.hboxlayout4.addWidget(self.addComponentLabel)

        self.addComponentBox = QtGui.QComboBox(self.detectorConfigBox)
        self.addComponentBox.setObjectName("addComponentBox")
        self.hboxlayout4.addWidget(self.addComponentBox)
        self.vboxlayout2.addLayout(self.hboxlayout4)
        self.hboxlayout.addLayout(self.vboxlayout2)

        self.expConfigBox = QtGui.QGroupBox(configdb)
        self.expConfigBox.setGeometry(QtCore.QRect(10,110,590,327))
        self.expConfigBox.setObjectName("expConfigBox")

        self.hboxlayout5 = QtGui.QHBoxLayout(self.expConfigBox)
        self.hboxlayout5.setObjectName("hboxlayout5")

        self.vboxlayout3 = QtGui.QVBoxLayout()
        self.vboxlayout3.setObjectName("vboxlayout3")

        self.configNameLabel = QtGui.QLabel(self.expConfigBox)
        self.configNameLabel.setAlignment(QtCore.Qt.AlignCenter)
        self.configNameLabel.setObjectName("configNameLabel")
        self.vboxlayout3.addWidget(self.configNameLabel)

        self.configList = QtGui.QListWidget(self.expConfigBox)
        self.configList.setObjectName("configList")
        self.vboxlayout3.addWidget(self.configList)

        self.hboxlayout6 = QtGui.QHBoxLayout()
        self.hboxlayout6.setObjectName("hboxlayout6")

        self.newConfigEdit = QtGui.QLineEdit(self.expConfigBox)
        self.newConfigEdit.setObjectName("newConfigEdit")
        self.hboxlayout6.addWidget(self.newConfigEdit)

        self.newConfigButton = QtGui.QPushButton(self.expConfigBox)
        self.newConfigButton.setObjectName("newConfigButton")
        self.hboxlayout6.addWidget(self.newConfigButton)
        self.vboxlayout3.addLayout(self.hboxlayout6)

        self.hboxlayout7 = QtGui.QHBoxLayout()
        self.hboxlayout7.setObjectName("hboxlayout7")

        self.copyConfigEdit = QtGui.QLineEdit(self.expConfigBox)
        self.copyConfigEdit.setObjectName("copyConfigEdit")
        self.hboxlayout7.addWidget(self.copyConfigEdit)

        self.copyConfigButton = QtGui.QPushButton(self.expConfigBox)
        self.copyConfigButton.setObjectName("copyConfigButton")
        self.hboxlayout7.addWidget(self.copyConfigButton)
        self.vboxlayout3.addLayout(self.hboxlayout7)
        self.hboxlayout5.addLayout(self.vboxlayout3)

        self.vboxlayout4 = QtGui.QVBoxLayout()
        self.vboxlayout4.setObjectName("vboxlayout4")

        self.detectorListLabel = QtGui.QLabel(self.expConfigBox)
        self.detectorListLabel.setAlignment(QtCore.Qt.AlignCenter)
        self.detectorListLabel.setObjectName("detectorListLabel")
        self.vboxlayout4.addWidget(self.detectorListLabel)

        self.detectorList = QtGui.QListWidget(self.expConfigBox)
        self.detectorList.setObjectName("detectorList")
        self.vboxlayout4.addWidget(self.detectorList)

        self.hboxlayout8 = QtGui.QHBoxLayout()
        self.hboxlayout8.setObjectName("hboxlayout8")

        self.label = QtGui.QLabel(self.expConfigBox)
        self.label.setAlignment(QtCore.Qt.AlignRight|QtCore.Qt.AlignTrailing|QtCore.Qt.AlignVCenter)
        self.label.setObjectName("label")
        self.hboxlayout8.addWidget(self.label)

        self.addDetectorBox = QtGui.QComboBox(self.expConfigBox)
        self.addDetectorBox.setObjectName("addDetectorBox")
        self.hboxlayout8.addWidget(self.addDetectorBox)
        self.vboxlayout4.addLayout(self.hboxlayout8)
        self.hboxlayout5.addLayout(self.vboxlayout4)

        spacerItem = QtGui.QSpacerItem(40,20,QtGui.QSizePolicy.Expanding,QtGui.QSizePolicy.Minimum)
        self.hboxlayout5.addItem(spacerItem)

        self.transactionBox = QtGui.QGroupBox(configdb)
        self.transactionBox.setGeometry(QtCore.QRect(10,5,276,92))
        self.transactionBox.setObjectName("transactionBox")

        self.vboxlayout5 = QtGui.QVBoxLayout(self.transactionBox)
        self.vboxlayout5.setObjectName("vboxlayout5")

        self.hboxlayout9 = QtGui.QHBoxLayout()
        self.hboxlayout9.setObjectName("hboxlayout9")

        self.clearButton = QtGui.QPushButton(self.transactionBox)
        self.clearButton.setObjectName("clearButton")
        self.hboxlayout9.addWidget(self.clearButton)

        self.commitButton = QtGui.QPushButton(self.transactionBox)
        self.commitButton.setObjectName("commitButton")
        self.hboxlayout9.addWidget(self.commitButton)

        self.updateKeysButton = QtGui.QPushButton(self.transactionBox)
        self.updateKeysButton.setObjectName("updateKeysButton")
        self.hboxlayout9.addWidget(self.updateKeysButton)
        self.vboxlayout5.addLayout(self.hboxlayout9)

        self.transactionHelp = QtGui.QLabel(self.transactionBox)
        self.transactionHelp.setObjectName("transactionHelp")
        self.vboxlayout5.addWidget(self.transactionHelp)

        self.retranslateUi(configdb)
        QtCore.QObject.connect(self.newConfigEdit,QtCore.SIGNAL("returnPressed()"),self.newConfigButton.animateClick)
        QtCore.QObject.connect(self.copyConfigEdit,QtCore.SIGNAL("returnPressed()"),self.copyConfigButton.animateClick)
        QtCore.QObject.connect(self.newDetectorEdit,QtCore.SIGNAL("returnPressed()"),self.newDetectorButton.animateClick)
        QtCore.QObject.connect(self.newConfigEdit_2,QtCore.SIGNAL("returnPressed()"),self.newConfigButton_2.animateClick)
        QtCore.QObject.connect(self.copyConfigEdit_2,QtCore.SIGNAL("returnPressed()"),self.copyConfigButton_2.animateClick)
        QtCore.QMetaObject.connectSlotsByName(configdb)

    def retranslateUi(self, configdb):
        configdb.setWindowTitle(QtGui.QApplication.translate("configdb", "Form", None, QtGui.QApplication.UnicodeUTF8))
        self.detectorConfigBox.setTitle(QtGui.QApplication.translate("configdb", "Devices Configuration", None, QtGui.QApplication.UnicodeUTF8))
        self.detectorLabel_2.setText(QtGui.QApplication.translate("configdb", "Device", None, QtGui.QApplication.UnicodeUTF8))
        self.newDetectorButton.setText(QtGui.QApplication.translate("configdb", "New", None, QtGui.QApplication.UnicodeUTF8))
        self.configLabel.setText(QtGui.QApplication.translate("configdb", "Config Name", None, QtGui.QApplication.UnicodeUTF8))
        self.newConfigButton_2.setText(QtGui.QApplication.translate("configdb", "New", None, QtGui.QApplication.UnicodeUTF8))
        self.copyConfigButton_2.setText(QtGui.QApplication.translate("configdb", "Copy To", None, QtGui.QApplication.UnicodeUTF8))
        self.componentLabel.setText(QtGui.QApplication.translate("configdb", "Component", None, QtGui.QApplication.UnicodeUTF8))
        self.addComponentLabel.setText(QtGui.QApplication.translate("configdb", "Add", None, QtGui.QApplication.UnicodeUTF8))
        self.expConfigBox.setTitle(QtGui.QApplication.translate("configdb", "Experiment Configuration", None, QtGui.QApplication.UnicodeUTF8))
        self.configNameLabel.setText(QtGui.QApplication.translate("configdb", "Config Name", None, QtGui.QApplication.UnicodeUTF8))
        self.newConfigButton.setText(QtGui.QApplication.translate("configdb", "New", None, QtGui.QApplication.UnicodeUTF8))
        self.copyConfigButton.setText(QtGui.QApplication.translate("configdb", "Copy To", None, QtGui.QApplication.UnicodeUTF8))
        self.detectorListLabel.setText(QtGui.QApplication.translate("configdb", "Device Configuration", None, QtGui.QApplication.UnicodeUTF8))
        self.label.setText(QtGui.QApplication.translate("configdb", "Add", None, QtGui.QApplication.UnicodeUTF8))
        self.transactionBox.setTitle(QtGui.QApplication.translate("configdb", "Database Transaction", None, QtGui.QApplication.UnicodeUTF8))
        self.clearButton.setText(QtGui.QApplication.translate("configdb", "Clear", None, QtGui.QApplication.UnicodeUTF8))
        self.commitButton.setText(QtGui.QApplication.translate("configdb", "Commit", None, QtGui.QApplication.UnicodeUTF8))
        self.updateKeysButton.setText(QtGui.QApplication.translate("configdb", "Update Keys", None, QtGui.QApplication.UnicodeUTF8))
        self.transactionHelp.setText(QtGui.QApplication.translate("configdb", "Pending transactions are indicated with *", None, QtGui.QApplication.UnicodeUTF8))

