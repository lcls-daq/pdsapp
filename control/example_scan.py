#!/reg/g/pcds/package/python-2.5.2/bin/python
#

import os
import struct
import socket
from optparse import OptionParser

class ControlPV:
    def __init__(self,name,value):
        self.name = name
        self.index = -1
        self.value = value

    def __init__(self,name,index,value):
        self.name = name
        self.index = index
        self.value = value

    def send(self,socket):
        socket.send(struct.pack('<32sId',self.name,self.index,self.value))

class MonitorPV:
    def __init__(self,name,lo,hi):
        self.name = name
        self.index = -1
        self.lo    = lo
        self.hi    = hi
        
    def __init__(self,name,index,lo,hi):
        self.name = name
        self.index = index
        self.lo    = lo
        self.hi    = hi
        
    def send(self,socket):
        socket.send(struct.pack('<32sIdd',self.name,self.index,self.lo,self.hi))
        
class DAQData:
    def __init__(self):
        self.control   = 0
        self.seconds   = 0
        self.nseconds  = 0
        self.controlpvs = []
        self.monitorpvs = []

    def settime(self,seconds,nseconds):
        self.control  = 0x40000000
        self.seconds  = seconds
        self.nseconds = nseconds
        
    def setevents(self,nevents):
        self.control  = 0x80000000 + nevents
        self.seconds  = 0
        self.nseconds = 0
        
    def addcontrol(self,pv):
        self.controlpvs.append(pv)

    def addmonitor(self,pv):
        self.monitorpvs.append(pv)
        
    def send(self,socket):
        print "execute"
        socket.send(struct.pack('<6I',
                                self.control,0,
                                self.nseconds,self.seconds,
                                len(self.controlpvs),
                                len(self.monitorpvs)))
        for item in self.controlpvs:
            item.send(socket)
        for item in self.monitorpvs:
            item.send(socket)

class DAQStatus:
    def __init__(self,s):
        self.value = struct.unpack('<i',s.recv(4))
        if self.value < 0:
            raise StandardError
        print "returned"
        
if __name__ == "__main__":
    import sys

    parser = OptionParser()
    parser.add_option("-a","--address",dest="host",default='',
                      help="connect to DAQ at HOST", metavar="HOST")
    parser.add_option("-p","--port",dest="port",default=10149,
                      help="connect to DAQ at PORT", metavar="PORT")

    (options, args) = parser.parse_args()
        
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

    s.connect((options.host,options.port))

#
#  Send the structure the first time to put the control variables
#    in the file header
#
    data = DAQData()
    data.addcontrol(ControlPV('EXAMPLEPV1',0))
    data.addcontrol(ControlPV('EXAMPLEPV2',0))
    data.send(s)
#
#  Wait for the DAQ to declare 'configured'
#
    result = DAQStatus(s)
        
#
#  Configure Cycle 1
#
    data = DAQData()
    data.setevents(100)
    data.addcontrol(ControlPV('EXAMPLEPV1',0))
    data.addcontrol(ControlPV('EXAMPLEPV2',0))
    data.send(s)
#
#  Wait for the DAQ to declare 'enabled'
#
    result = DAQStatus(s)
#
#  Enable the EVR sequence
#

#
#  Wait for the DAQ to declare 'disabled'
#
    result = DAQStatus(s)
#
#  Disable the EVR sequence
#

#  Cycle 2
    data = DAQData()
    data.setevents(100)
    data.addcontrol(ControlPV('EXAMPLEPV1',1))
    data.addcontrol(ControlPV('EXAMPLEPV2',0))
    data.send(s)
    result = DAQStatus(s)
#  Enable the sequence
    result = DAQStatus(s)
#  Disable the sequence

#  Cycle 3
    data = DAQData()
    data.setevents(100)
    data.addcontrol(ControlPV('EXAMPLEPV1',1))
    data.addcontrol(ControlPV('EXAMPLEPV2',1))
    data.send(s)
    result = DAQStatus(s)
#  Enable the sequence
    result = DAQStatus(s)
#  Disable the sequence

    s.close()
