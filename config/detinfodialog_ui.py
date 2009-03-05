# -*- coding: utf-8 -*-

# Form implementation generated from reading ui file 'pdsapp/config/detinfodialog.ui'
#
# Created: Mon Feb 23 14:58:11 2009
#      by: PyQt4 UI code generator 4.3.3
#
# WARNING! All changes made in this file will be lost!

from PyQt4 import QtCore, QtGui

class Ui_detInfoDialog(object):
    def setupUi(self, detInfoDialog):
        detInfoDialog.setObjectName("detInfoDialog")
        detInfoDialog.resize(QtCore.QSize(QtCore.QRect(0,0,653,357).size()).expandedTo(detInfoDialog.minimumSizeHint()))

        self.widget = QtGui.QWidget(detInfoDialog)
        self.widget.setGeometry(QtCore.QRect(9,9,599,308))
        self.widget.setObjectName("widget")

        self.vboxlayout = QtGui.QVBoxLayout(self.widget)
        self.vboxlayout.setObjectName("vboxlayout")

        self.hboxlayout = QtGui.QHBoxLayout()
        self.hboxlayout.setObjectName("hboxlayout")

        self.detInfoBox = QtGui.QGroupBox(self.widget)
        self.detInfoBox.setObjectName("detInfoBox")

        self.vboxlayout1 = QtGui.QVBoxLayout(self.detInfoBox)
        self.vboxlayout1.setObjectName("vboxlayout1")

        self.gridlayout = QtGui.QGridLayout()
        self.gridlayout.setObjectName("gridlayout")

        self.detInfoDetLabel = QtGui.QLabel(self.detInfoBox)
        self.detInfoDetLabel.setAlignment(QtCore.Qt.AlignRight|QtCore.Qt.AlignTrailing|QtCore.Qt.AlignVCenter)
        self.detInfoDetLabel.setObjectName("detInfoDetLabel")
        self.gridlayout.addWidget(self.detInfoDetLabel,0,0,1,1)

        self.detInfoDetBox = QtGui.QComboBox(self.detInfoBox)
        self.detInfoDetBox.setObjectName("detInfoDetBox")
        self.gridlayout.addWidget(self.detInfoDetBox,0,1,1,1)

        self.detInfoDetIdLabel = QtGui.QLabel(self.detInfoBox)
        self.detInfoDetIdLabel.setAlignment(QtCore.Qt.AlignRight|QtCore.Qt.AlignTrailing|QtCore.Qt.AlignVCenter)
        self.detInfoDetIdLabel.setObjectName("detInfoDetIdLabel")
        self.gridlayout.addWidget(self.detInfoDetIdLabel,0,2,1,1)

        self.detInfoDetIdEdit = QtGui.QLineEdit(self.detInfoBox)
        self.detInfoDetIdEdit.setObjectName("detInfoDetIdEdit")
        self.gridlayout.addWidget(self.detInfoDetIdEdit,0,3,1,1)

        self.detInfoDevLabel = QtGui.QLabel(self.detInfoBox)
        self.detInfoDevLabel.setAlignment(QtCore.Qt.AlignRight|QtCore.Qt.AlignTrailing|QtCore.Qt.AlignVCenter)
        self.detInfoDevLabel.setObjectName("detInfoDevLabel")
        self.gridlayout.addWidget(self.detInfoDevLabel,1,0,1,1)

        self.detInfoDevBox = QtGui.QComboBox(self.detInfoBox)
        self.detInfoDevBox.setObjectName("detInfoDevBox")
        self.gridlayout.addWidget(self.detInfoDevBox,1,1,1,1)

        self.detInfoDevIdLabel = QtGui.QLabel(self.detInfoBox)
        self.detInfoDevIdLabel.setAlignment(QtCore.Qt.AlignRight|QtCore.Qt.AlignTrailing|QtCore.Qt.AlignVCenter)
        self.detInfoDevIdLabel.setObjectName("detInfoDevIdLabel")
        self.gridlayout.addWidget(self.detInfoDevIdLabel,1,2,1,1)

        self.detInfoDevIdEdit = QtGui.QLineEdit(self.detInfoBox)
        self.detInfoDevIdEdit.setObjectName("detInfoDevIdEdit")
        self.gridlayout.addWidget(self.detInfoDevIdEdit,1,3,1,1)
        self.vboxlayout1.addLayout(self.gridlayout)

        spacerItem = QtGui.QSpacerItem(20,40,QtGui.QSizePolicy.Minimum,QtGui.QSizePolicy.Expanding)
        self.vboxlayout1.addItem(spacerItem)

        self.hboxlayout1 = QtGui.QHBoxLayout()
        self.hboxlayout1.setObjectName("hboxlayout1")

        spacerItem1 = QtGui.QSpacerItem(40,20,QtGui.QSizePolicy.Expanding,QtGui.QSizePolicy.Minimum)
        self.hboxlayout1.addItem(spacerItem1)

        self.addButton = QtGui.QPushButton(self.detInfoBox)
        self.addButton.setObjectName("addButton")
        self.hboxlayout1.addWidget(self.addButton)

        spacerItem2 = QtGui.QSpacerItem(40,20,QtGui.QSizePolicy.Expanding,QtGui.QSizePolicy.Minimum)
        self.hboxlayout1.addItem(spacerItem2)
        self.vboxlayout1.addLayout(self.hboxlayout1)
        self.hboxlayout.addWidget(self.detInfoBox)

        self.detInfoList = QtGui.QGroupBox(self.widget)
        self.detInfoList.setObjectName("detInfoList")

        self.vboxlayout2 = QtGui.QVBoxLayout(self.detInfoList)
        self.vboxlayout2.setObjectName("vboxlayout2")

        self.listWidget = QtGui.QListWidget(self.detInfoList)
        self.listWidget.setObjectName("listWidget")
        self.vboxlayout2.addWidget(self.listWidget)

        self.hboxlayout2 = QtGui.QHBoxLayout()
        self.hboxlayout2.setObjectName("hboxlayout2")

        spacerItem3 = QtGui.QSpacerItem(40,20,QtGui.QSizePolicy.Expanding,QtGui.QSizePolicy.Minimum)
        self.hboxlayout2.addItem(spacerItem3)

        self.removeButton = QtGui.QPushButton(self.detInfoList)
        self.removeButton.setObjectName("removeButton")
        self.hboxlayout2.addWidget(self.removeButton)

        spacerItem4 = QtGui.QSpacerItem(40,20,QtGui.QSizePolicy.Expanding,QtGui.QSizePolicy.Minimum)
        self.hboxlayout2.addItem(spacerItem4)
        self.vboxlayout2.addLayout(self.hboxlayout2)
        self.hboxlayout.addWidget(self.detInfoList)
        self.vboxlayout.addLayout(self.hboxlayout)

        self.buttonBox = QtGui.QDialogButtonBox(self.widget)
        self.buttonBox.setOrientation(QtCore.Qt.Horizontal)
        self.buttonBox.setStandardButtons(QtGui.QDialogButtonBox.Cancel|QtGui.QDialogButtonBox.NoButton|QtGui.QDialogButtonBox.Ok)
        self.buttonBox.setObjectName("buttonBox")
        self.vboxlayout.addWidget(self.buttonBox)

        self.retranslateUi(detInfoDialog)
        QtCore.QObject.connect(self.buttonBox,QtCore.SIGNAL("accepted()"),detInfoDialog.accept)
        QtCore.QObject.connect(self.buttonBox,QtCore.SIGNAL("rejected()"),detInfoDialog.reject)
        QtCore.QMetaObject.connectSlotsByName(detInfoDialog)

    def retranslateUi(self, detInfoDialog):
        detInfoDialog.setWindowTitle(QtGui.QApplication.translate("detInfoDialog", "Dialog", None, QtGui.QApplication.UnicodeUTF8))
        self.detInfoBox.setTitle(QtGui.QApplication.translate("detInfoDialog", "Detector Info", None, QtGui.QApplication.UnicodeUTF8))
        self.detInfoDetLabel.setText(QtGui.QApplication.translate("detInfoDialog", "Detector", None, QtGui.QApplication.UnicodeUTF8))
        self.detInfoDetIdLabel.setText(QtGui.QApplication.translate("detInfoDialog", "Id", None, QtGui.QApplication.UnicodeUTF8))
        self.detInfoDevLabel.setText(QtGui.QApplication.translate("detInfoDialog", "Device", None, QtGui.QApplication.UnicodeUTF8))
        self.detInfoDevIdLabel.setText(QtGui.QApplication.translate("detInfoDialog", "Id", None, QtGui.QApplication.UnicodeUTF8))
        self.addButton.setText(QtGui.QApplication.translate("detInfoDialog", "Add", None, QtGui.QApplication.UnicodeUTF8))
        self.detInfoList.setTitle(QtGui.QApplication.translate("detInfoDialog", "List", None, QtGui.QApplication.UnicodeUTF8))
        self.removeButton.setText(QtGui.QApplication.translate("detInfoDialog", "Remove", None, QtGui.QApplication.UnicodeUTF8))

