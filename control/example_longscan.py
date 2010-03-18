#!/reg/g/pcds/package/python-2.5.2/bin/python
#

import os
import struct
import socket
from optparse import OptionParser

class ControlPV:
    def __init__(self,name,value):
        self.index = -1
        list = name.rstrip("]").split("[")
        if (len(list)>2):
            raise StandardError
        self.name = list[0]
        if (len(list)>1):
            self.index = int(list[1])
        self.value = value

    def send(self,socket):
        socket.send(struct.pack('<32sid',self.name,self.index,self.value))

class MonitorPV:
    def __init__(self,name,lo,hi):
        self.index = -1        
        list = name.rstrip("]").split("[")
        if (len(list)>2):
            raise StandardError
        self.name = list[0]
        if (len(list)>1):
            self.index = int(list[1])
        self.lo    = lo
        self.hi    = hi
        
    def send(self,socket):
        socket.send(struct.pack('<32sidd',self.name,self.index,self.lo,self.hi))
        
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
        if self.value[0] < 0:
            print "status = ", self.value[0]
            raise StandardError
        
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
    data.setevents(0)
    data.addcontrol(ControlPV('EXAMPLEPV1',0))
    data.addcontrol(ControlPV('EXAMPLEPV2',0))
    data.send(s)
#
#  Wait for the DAQ to declare 'configured'
#
    result = DAQStatus(s)
    print "Configured."

    for cycle in range(100):
        data = DAQData()
        data.setevents(100)
        data.addcontrol(ControlPV('EXAMPLEPV1',cycle))
        data.addcontrol(ControlPV('EXAMPLEPV2',100-cycle))

        print "Cycle ", cycle
        data.send(s)

        result = DAQStatus(s)  # wait for enabled , then enable the EVR sequence

        result = DAQStatus(s)  # wait for disabled, then disable the EVR sequence
            
    s.close()
