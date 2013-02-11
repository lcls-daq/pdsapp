#!/usr/bin/env /reg/common/package/python/2.5.5/bin/python

from options import Options
from PyQt4 import QtCore, QtGui
import sys
import socket
import os

keepalive = 60

class DummyCheckBox(QtGui.QLabel):
    def __init__(self, parent):
        QtGui.QLabel.__init__(self, parent)

    def isChecked(self):
        return False

class Ui_MainWindow(object):
    def add_camera(self, name, host, cfg):
        if (name == "DUMMY"):
            c = DummyCheckBox(self.cameraBox)
            c.setText(" ")
        else:
            c = QtGui.QCheckBox(self.cameraBox)
            c.setText(name.replace('_', ' '))
        c.host = host
        c.cfg = cfg
        c.name = name
        l = len(self.cameras)
        self.cameraLayout.addWidget(c, l / self.cpr, l % self.cpr, 1, 1)
        self.cameras.append(c)
        self.cameraBox.setVisible(True)

    def add_bld(self, name, host, cfg):
        if (name == "DUMMY"):
            c = DummyCheckBox(self.cameraBox)
            c.setText(" ")
        else:
            c = QtGui.QCheckBox(self.bldBox)
            c.setText(name.replace('_', ' '))
        c.host = host
        c.cfg = cfg
        c.name = name
        l = len(self.blds)
        self.bldLayout.addWidget(c, l / self.bpr, l % self.bpr, 1, 1)
        self.blds.append(c)
        self.bldBox.setVisible(True)

    def add_pv(self, name, host, cfg):
        if (name == "DUMMY"):
            c = DummyCheckBox(self.cameraBox)
            c.setText(" ")
        else:
            c = QtGui.QCheckBox(self.pvBox)
            c.setText(name.replace('_', ' '))
        c.host = host
        c.cfg = cfg
        c.name = name
        l = len(self.pvs)
        self.pvLayout.addWidget(c, l / self.ppr, l % self.ppr, 1, 1)
        self.pvs.append(c)
        self.pvBox.setVisible(True)

    def get_camera(self, d, w):
        for c in self.cameras:
            if c.isChecked():
                if (c.host == "*"):
                    w.append(c.cfg)
                    w.append("record " + c.name)
                else:
                    try:
                        cur = d[c.host]
                        cur.append(c.cfg)
                        cur.append("record " + c.name)
                    except:
                        d[c.host] = [c.cfg, "record " + c.name]

    def get_bld(self, d, w):
        for c in self.blds:
            if c.isChecked():
                if (c.host == "*"):
                    w.append(c.cfg)
                    w.append("record " + c.name)
                else:
                    try:
                        cur = d[c.host]
                        cur.append(c.cfg)
                        cur.append("record " + c.name)
                    except:
                        d[c.host] = [c.cfg, "record " + c.name]

    def get_pv(self, d, w):
        for c in self.pvs:
            if c.isChecked():
                if (c.host == "*"):
                    w.append(c.cfg)
                    w.append("record " + c.name)
                else:
                    try:
                        cur = d[c.host]
                        cur.append(c.cfg)
                        cur.append("record " + c.name)
                    except:
                        d[c.host] = [c.cfg, "record " + c.name]
        
    def __init__(self, MainWindow):
        self.cameras = []
        self.blds    = []
        self.pvs     = []
        self.cpr     = 3
        self.bpr     = 3
        self.ppr     = 3
        
        MainWindow.setObjectName("MainWindow")
        MainWindow.resize(900, 100)
        MainWindow.setWindowTitle("Controls Recorder Tool")
        sizePolicy = QtGui.QSizePolicy(QtGui.QSizePolicy.Minimum, QtGui.QSizePolicy.Minimum)
        sizePolicy.setHorizontalStretch(0)
        sizePolicy.setVerticalStretch(0)
        sizePolicy.setHeightForWidth(MainWindow.sizePolicy().hasHeightForWidth())
        MainWindow.setSizePolicy(sizePolicy)

        self.centralwidget = QtGui.QWidget(MainWindow)
        self.centralwidget.setObjectName("centralwidget")
        self.mainLayout = QtGui.QVBoxLayout(self.centralwidget)
        self.mainLayout.setObjectName("mainLayout")

        self.controlBox = QtGui.QGroupBox(self.centralwidget)
        sizePolicy = QtGui.QSizePolicy(QtGui.QSizePolicy.MinimumExpanding, QtGui.QSizePolicy.Fixed)
        sizePolicy.setHorizontalStretch(0)
        sizePolicy.setVerticalStretch(0)
        sizePolicy.setHeightForWidth(self.controlBox.sizePolicy().hasHeightForWidth())
        self.controlBox.setSizePolicy(sizePolicy)
        self.controlBox.setMinimumSize(QtCore.QSize(0, 0))
        self.controlBox.setMaximumSize(QtCore.QSize(16777215, 16777215))
        self.controlBox.setSizePolicy(sizePolicy)
        self.controlBox.setTitle("Controls")
        self.controlBox.setObjectName("controlBox")
        self.controlLayout = QtGui.QGridLayout(self.controlBox)
        self.controlLayout.setObjectName("controlLayout")

        self.record = QtGui.QPushButton(self.controlBox)
        self.record.setText("Record")
        self.record.setCheckable(True)
        sizePolicy = QtGui.QSizePolicy(QtGui.QSizePolicy.Fixed, QtGui.QSizePolicy.Fixed)
        sizePolicy.setHorizontalStretch(0)
        sizePolicy.setVerticalStretch(0)
        sizePolicy.setHeightForWidth(self.record.sizePolicy().hasHeightForWidth())
        self.record.setSizePolicy(sizePolicy)
        self.record.setMinimumSize(QtCore.QSize(80, 80))
        self.record.setMaximumSize(QtCore.QSize(80, 80))
        self.record.setObjectName("record")
        self.controlLayout.addWidget(self.record, 0, 0, 1, 1)

        self.label_2 = QtGui.QLabel(self.controlBox)
        self.label_2.setText("Record Time (sec):")
        self.label_2.setObjectName("label_2")
        self.controlLayout.addWidget(self.label_2, 0, 1, 1, 1)

        self.recordTime = QtGui.QLineEdit(self.controlBox)
        self.recordTime.setObjectName("recordTime")
        self.controlLayout.addWidget(self.recordTime, 0, 2, 1, 1)

        self.clearLog = QtGui.QPushButton(self.controlBox)
        self.clearLog.setObjectName("clearLog")
        self.clearLog.setText("Clear Log")
        self.controlLayout.addWidget(self.clearLog, 0, 3, 1, 1)

        self.status = QtGui.QPlainTextEdit(self.controlBox)
        font = QtGui.QFont()
        font.setFamily("Monospace")
        font.setPointSize(10)
        self.status.setFont(font)
        self.status.setProperty("cursor", QtCore.Qt.IBeamCursor)
        self.status.ensureCursorVisible()
        self.status.setUndoRedoEnabled(False)
        self.status.setReadOnly(True)
        self.status.setLineWrapMode(QtGui.QPlainTextEdit.NoWrap)
        self.status.setMaximumBlockCount(500)
        self.status.setPlainText("Starting up...")
        self.status.setObjectName("status")
        self.controlLayout.addWidget(self.status, 1, 0, 1, 4)

        self.mainLayout.addWidget(self.controlBox)

        self.cameraBox = QtGui.QGroupBox(self.centralwidget)
        self.cameraBox.setTitle("Cameras")
        self.cameraBox.setObjectName("cameraBox")
        self.cameraBox.setVisible(False)
        self.cameraLayout = QtGui.QGridLayout(self.cameraBox)
        self.cameraLayout.setObjectName("cameraLayout")
        
        self.mainLayout.addWidget(self.cameraBox)
        
        self.bldBox = QtGui.QGroupBox(self.centralwidget)
        self.bldBox.setTitle("Beam Line Data")
        self.bldBox.setObjectName("bldBox")
        self.bldBox.setVisible(False)
        self.bldLayout = QtGui.QGridLayout(self.bldBox)
        self.bldLayout.setObjectName("bldLayout")
        
        self.mainLayout.addWidget(self.bldBox)
        
        self.pvBox = QtGui.QGroupBox(self.centralwidget)
        self.pvBox.setTitle("Process Variables")
        self.pvBox.setObjectName("groupBox")
        self.pvBox.setVisible(False)
        self.pvLayout = QtGui.QGridLayout(self.pvBox)
        self.pvLayout.setObjectName("pvLayout")

        self.mainLayout.addWidget(self.pvBox)

        MainWindow.setCentralWidget(self.centralwidget)

        #self.menubar = QtGui.QMenuBar(MainWindow)
        #self.menubar.setGeometry(QtCore.QRect(0, 0, 326, 23))
        #self.menubar.setObjectName("menubar")
        #MainWindow.setMenuBar(self.menubar)
        #self.statusbar = QtGui.QStatusBar(MainWindow)
        #self.statusbar.setObjectName("statusbar")
        #MainWindow.setStatusBar(self.statusbar)

        QtCore.QMetaObject.connectSlotsByName(MainWindow)

        
class GraphicUserInterface(QtGui.QMainWindow):
    def log(self, s):
        self.ui.status.appendPlainText(s)
        
    def __init__(self, app, config):
        QtGui.QMainWindow.__init__(self)
        self.__app = app
        self.notifiers = {}
        self.defhost = ""
        self.defport = 9999
        self.timer = QtCore.QTimer()
        self.user = os.getenv("USER")
        self.runfile = os.getenv("HOME") + "/.camrecordrc"
        self.current_run = ""

        self.ui = Ui_MainWindow(self)

        self.read_config(config)

        self.connect(self.ui.record, QtCore.SIGNAL("clicked()"), self.doRecord)
        self.connect(self.ui.clearLog, QtCore.SIGNAL("clicked()"), self.doClearLog)
        self.connect(self.timer, QtCore.SIGNAL("timeout()"), self.doTimer)
        if keepalive != 0:
            self.timer.start(keepalive * 1000)

        self.log("Initialized!")

    def read_config(self, config):
        fp=open(config)
        lns=fp.readlines()
        fp.close()
        host = "*"
        for ln in lns:
            ln = ln.strip()
            if ln == "" or ln.startswith("#"):
                continue
            sln=ln.split()
            kind = sln[0].strip()
            if (kind == "defhost"):
                self.defhost = sln[1].strip()
            elif (kind == "defport"):
                self.defport = int(sln[1].strip())
            elif (kind == "host"):
                host = sln[1].strip()
                if not ((":" in host) or (host == "*")):
                    host = host + ":" + str(self.defport)
            elif (kind == "camera-per-row"):
                try:
                    self.ui.cpr = int(sln[1].strip())
                except:
                    pass
                if (self.ui.cpr <= 0):
                    self.ui.cpr = 1
            elif (kind == "bld-per-row"):
                try:
                    self.ui.bpr = int(sln[1].strip())
                except:
                    pass
                if (self.ui.bpr <= 0):
                    self.ui.bpr = 1
            elif (kind == "pv-per-row"):
                try:
                    self.ui.ppr = int(sln[1].strip())
                except:
                    pass
                if (self.ui.ppr <= 0):
                    self.ui.ppr = 1
            elif (kind == "camera"):
                self.ui.add_camera(sln[1].strip(), host, ln)
            elif (kind == "bld"):
                self.ui.add_bld(sln[1].strip(), host, ln)
            elif (kind == "pv"):
                self.ui.add_pv(sln[1].strip(), host, ln)

    def dumpConfig(self, d, w):
        for host, cfg in d.items():
            self.log("Host %s:" % host)
            for ln in cfg:
                self.log(ln)
            self.log("")
        self.log("Wildcard:")
        for ln in w:
            self.log(ln)
        self.log("")

    def sendConfig(self, sock, cfg):
        for ln in cfg:
            sock.sendall(ln + "\n");

    def getRunNo(self):
        try:
            fp=open(self.runfile)
            run=int(fp.readline().strip())
            fp.close()
        except:
            run = 1
        try:
            fp=open(self.runfile, 'w')
            fp.write(str(run+1))
        except:
            pass
        return run

    def doRecord(self):
        if self.ui.record.isChecked():
            # Start to record
            self.ui.record.setText("Stop!")
            d = {}
            w = []
            self.ui.get_camera(d, w)
            self.ui.get_bld(d, w)
            self.ui.get_pv(d, w)
            # self.dumpConfig(d, w)
            try:
                t = int(self.ui.recordTime.text())
                self.log("Recording for %d seconds." % t)
            except:
                t = 0
            keys = d.keys()
            if keys == [] and w == []:
                self.log("Nothing to record!")
                self.ui.record.setChecked(False)
                self.doRecord()
                return
            if keys == [] and w != []:
                # Use the default host!
                d[self.defhost + ":" + str(self.defport)] = w
            else:
                # Do we want to spread these around?!?
                d[keys[0]].extend(w)
            self.current_run = "%04d" % self.getRunNo()
            for host in d.keys():
                hp = host.split(":")
                sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
                try:
                    sock.settimeout(1.0)
                    print "Connecting to %s:%d" % (hp[0], int(hp[1]))
                    sock.connect((hp[0], int(hp[1])))
                    sock.sendall("hostname " + hp[0] + "\n")
                    sock.sendall("timeout " + str(t) + "\n")
                    sock.sendall("keepalive " + str(int(keepalive * 3 / 2)) + "\n")
                    #
                    # Generate a real filename here!
                    #
                    sock.sendall("output " + self.user + "/" + hp[0] + "/e000-r" + self.current_run + "\n")
                    #
                    # Send the wildcards to the first.  We might want to spread
                    # them out somehow.
                    #
                    self.sendConfig(sock, d[host])
                    fd = sock.fileno()
                    notifier = QtCore.QSocketNotifier(fd, QtCore.QSocketNotifier.Read)
                    self.notifiers[fd] = (notifier, sock)
                    self.connect(notifier, QtCore.SIGNAL('activated(int)'), self.doSocketInput)
                except:
                    self.log("Cannot connect to %s!" % host)
                    self.ui.record.setChecked(False)
                    self.doRecord()
                    return
            self.log("Beginning to record run " + self.current_run)
            for fd, ntuple in self.notifiers.items():
                try:
                    ntuple[1].sendall("end" + "\n");
                except:
                    pass
        else:
            # Stop recording
            self.ui.record.setText("Record")
            if len(self.notifiers) != 0:
                self.log("Stopping recording of run " + self.current_run)
                for fd, ntuple in self.notifiers.items():
                    try:
                        ntuple[1].sendall("stop" + "\n");
                    except:
                        self.log("Problem sending stop?!?")
            elif self.current_run != "":
                self.log("Run " + self.current_run + " complete.")
                self.current_run = ""

    def doClearLog(self):
        self.ui.status.setPlainText("Log cleared.")

    def doTimer(self):
        for fd, ntuple in self.notifiers.items():
            try:
                ntuple[1].sendall("\n");
            except:
                pass

    def doSocketInput(self, fd):
        notifier, sock = self.notifiers[fd]
        try:
            data = sock.recv(4096)
            if data == "":
                # Must be EOF!
                #self.log("Closed fd %d on EOF" % fd)
                sock.close()
                del self.notifiers[fd]
            else:
                self.log(data.strip())
        except:
            #self.log("Closed fd %d on exception" % fd)
            sock.close()
            del self.notifiers[fd]
        if len(self.notifiers) == 0:
            self.ui.record.setChecked(False)
            self.doRecord()
            
    def shutdown(self):
        print "Done!"

def crrun(config):
  app = QtGui.QApplication([''])
  gui = GraphicUserInterface(app, config)
  try:
#    sys.setcheckinterval(1000) # default is 100
    gui.show()
    retval = app.exec_()
  except KeyboardInterrupt:
    app.exit(1)
    retval = 1
  gui.shutdown()
  return retval

if __name__ == '__main__':
  options = Options(['config'], [], [])
  try:
    options.parse()
  except Exception, msg:
    options.usage(str(msg))
    sys.exit()
  if options.port != None:
      port = int(options.port)
  else:
      port = 9999
  if options.port != None:
      port = int(options.port)
  else:
      port = 9999
  retval = crrun(options.config)
  sys.exit(retval)
