#!/reg/g/pcds/package/python-2.5.2/bin/python
#

import os
import struct
import socket

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

class DAQKey:
    def __init__(self,s):
        self.socket = s
        self.value = struct.unpack('<i',self.socket.recv(4))[0]
        if self.value < 0:
            raise StandardError

    def set(self,v):
        self.value = v
        self.socket.send(struct.pack('<i',self.value))
        
class DAQStatus:
    def __init__(self,s):
        self.value = struct.unpack('<i',s.recv(4))
        if self.value[0] < 0:
            raise StandardError
        
