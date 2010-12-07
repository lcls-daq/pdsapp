#!/reg/g/pcds/package/python-2.5.2/bin/python
#

import struct

DetInfo = '00000100'

TypeId  = '00040008'

EvrClk  = 119.e6

class EventCodeV5:
    def __init__(self):
        self.code = 0
        self.maskeventattr = 0
        self.reportdelay = 0
        self.reportwidth = 1
        self.masktrigger = 0
        self.maskset     = 0
        self.maskclear   = 0
        self.description = ''
        
    def read(self,f):
        (self.code,self.maskeventattr, \
         self.reportdelay,self.reportwidth, \
         self.masktrigger,self.maskset,self.maskclear,
         self.description) \
         = struct.unpack(self.fmt(),f.read(struct.calcsize(self.fmt())))
        return self

    def write(self,f):
        f.write(struct.pack(self.fmt(),
                            self.code,
                            self.maskeventattr,
                            self.reportdelay,
                            self.reportwidth,
                            self.masktrigger,
                            self.maskset,
                            self.maskclear,
                            self.description))
        
    def fmt(self):
        return '<HHIIIII16s'

class PulseConfigV3:
    def __init__(self):
        self.pulseid  =  0
        self.polarity =  0
        self.delay    =  0
        self.width    =  0
        self.prescale =  1
        
    def read(self,f):
        (self.pulseid,self.polarity,self.prescale,self.delay,self.width) \
        = struct.unpack(self.fmt(),f.read(struct.calcsize(self.fmt())))
        return self
    
    def write(self,f):
        f.write(struct.pack(self.fmt(),
                            self.pulseid,self.polarity,self.prescale,self.delay,self.width))

    def fmt(self):
        return '<HHIII'

class OutputMap:
    def __init__(self):
        self.value = 0
        
    def read(self,f):
        self.value = struct.unpack(self.fmt(),f.read(struct.calcsize(self.fmt())))[0]
        return self
    
    def write(self,f):
        f.write(struct.pack(self.fmt(),self.value))

    def fmt(self):
        return '<I'

class SeqEntry:
    def __init__(self):
        self.eventcode = 0
        self.delay     = 0

    def read(self,f):
        value = struct.unpack(self.fmt(),f.read(struct.calcsize(self.fmt())))[0]
        self.eventcode = value>>24
        self.delay     = value&0xffffff

    def write(self,f):
        value = (self.eventcode<<24) | self.delay
        f.write(struct.pack(self.fmt(),value))

    def fmt(self):
        return '<I'

class SeqConfigV1:
    def __init__(self):
        # [0:7] = [120Hz, 60Hz, 30Hz, 10Hz, 5Hz, 1Hz, 0.5Hz, Disabled]
        self.source = 7
        self.length = 0
        self.cycles = 0
        self.entries = []

    def read(self,f):
        (self.source,self.length,self.cycles) \
        = struct.unpack(self.fmt(),f.read(struct.calcsize(self.fmt())))

        self.entries = []
        for i in range(self.length):
            self.entries.append(SeqEntry().read(f))

    def write(self,f):
        f.write(struct.pack(self.fmt(),
                            self.source,self.length,self.cycles))

        for i in range(self.entries):
            self.entries[i].write(f)

    def fmt(self):
        return '<III'
        
class ConfigV5:
    def __init__(self):
        self.neventcodes = 0
        self.npulses = 0
        self.noutputs = 0
        self.eventcodes = []
        self.pulses = []
        self.outputs = []
        self.seq_config = SeqConfigV1()
        
    def read(self,name):
        f = open(name,'r')

        (self.neventcodes,self.npulses,self.noutputs) \
        = struct.unpack(self.fmt(),f.read(struct.calcsize(self.fmt())))

        print self.neventcodes, self.npulses, self.noutputs
        
        self.eventcodes = []
        for i in range(self.neventcodes):
            self.eventcodes.append(EventCodeV4().read(f))
            
        self.pulses = []
        for i in range(self.npulses):
            self.pulses.append(PulseConfigV3().read(f))
            
        self.outputs = []
        for i in range(self.noutputs):
            self.outputs.append(OutputMap().read(f))

        self.seq_config = SeqConfigV1().read(f)

        f.close()

    def write(self,f):
        f.write(struct.pack(self.fmt(),
                            self.neventcodes,self.npulses,self.noutputs))

        for i in range(self.neventcodes):
            self.eventcodes[i].write(f)
            
        for i in range(self.npulses):
            self.pulses[i].write(f)
            
        for i in range(self.noutputs):
            self.outputs[i].write(f)

        self.seq_config.write(f)
        
    def fmt(self):
        return '<III'
