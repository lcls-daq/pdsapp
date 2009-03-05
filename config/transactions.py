#!/pcds/package/python-2.5.2/bin/python

#
#  Database Transactions
#

from PyQt4 import QtCore

class Ui_transactions:
    def __init__(self,db,ui,parent):
        self.db=db
        self.ui=ui
        parent.connect(ui.clearButton,
                       QtCore.SIGNAL("clicked()"),
                       self.db_clear)
        parent.connect(ui.commitButton,
                       QtCore.SIGNAL("clicked()"),
                       self.db_commit)
        parent.connect(ui.updateKeysButton,
                       QtCore.SIGNAL("clicked()"),
                       self.db_update_keys)

    def db_clear(self):
        self.db.read()
        pass     # restart UI

    def db_commit(self):
        self.db.write()

    #  Make sure that commit is done first
    def db_update_keys(self):
        self.db.update_keys()
