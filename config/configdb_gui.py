#!/pcds/package/python-2.5.2/bin/python

from PyQt4 import QtGui,QtCore
from configdb_ui import Ui_configdb
from CfgExpt import CfgExpt
from transactions import Ui_transactions
from experiment import Ui_experiment
from devices import Ui_devices

#
#  Device Configuration
#

#
#  User Interface Wrapper
#
class Ui_configdb_impl(QtGui.QWidget):
    def __init__(self,dbroot):
        QtGui.QWidget.__init__(self)

        #  First validate database
        self.db=CfgExpt(dbroot)
        if not self.db.is_valid():
            if QtGui.QMessageBox.question(self,"Configuration Database",
                                          "No valid database was found at " +
                                          dbroot + ".\n"
                                          "Create one?",
                                          QtGui.QMessageBox.Ok |
                                          QtGui.QMessageBox.Cancel):
                self.db.create()
            else:
                sys.exit(1)
        else:
            self.db.read()

        #
        #  Setup configdb UI
        #
        self.ui=Ui_configdb()
        self.ui.setupUi(self)

        #
        #  Split up the child widget implementations
        #
        self.transactions=Ui_transactions(self.db,self.ui,self)
        self.experiment  =Ui_experiment  (self.db,self.ui,self)
        self.devices     =Ui_devices     (self.db,self.ui,self)
        

def print_help(arg):
    print "Usage: " + arg + " <db root> "
    
if __name__ == "__main__":
    import sys

    if len(sys.argv)<2:
        print_help()
        sys.exit(1)

    app = QtGui.QApplication([])
    window = Ui_configdb_impl(sys.argv[1])

    window.show()
    sys.exit(app.exec_())
