#!/reg/g/pcds/package/python-2.5.2/bin/python
#

import struct

class ConfigV1:
    def __init__(self,name):
        f = open(name,'r')
        (self.triggerCounter, self.serialID, \
         self.chargeAmpRange, self.calibrationRange, \
         self.resetLength, self.resetDelay, \
         self.chargeAmpRefVoltage, self.calibrationVoltage, self.diodeBias, \
         self.status, self.errors, self.calStrobeLength, self.reserved, \
         self.trigDelay) \
         = struct.unpack(self.fmt(),f.read(struct.calcsize(self.fmt())))
        f.close()

    def write(self,name):
        f = open(name,'w')
        f.write(struct.pack(self.fmt(),
                            self.triggerCounter, self.serialID,
                            self.chargeAmpRange, self.calibrationRange,
                            self.resetLength, self.resetDelay, 
                            self.chargeAmpRefVoltage, self.calibrationVoltage, self.diodeBias, 
                            self.status, self.errors, self.calStrobeLength, self.reserved, 
                            self.trigDelay))
        f.close()
        
    def fmt(self):
        return '<QQHHIIfffHHHHI'

    def setChargeAmpRange_1pF(self,channel):
        self.setChargeAmpRange(channel,0)
        
    def setChargeAmpRange_100pF(self,channel):
        self.setChargeAmpRange(channel,1)
        
    def setChargeAmpRange_10nF(self,channel):
        self.setChargeAmpRange(channel,2)
        
    def setChargeAmpRange(self,channel,value):
        v  = self.chargeAmpRange & ~(0x3<<(2*channel))
        v |= value<<(2*channel)
        self.chargeAmpRange = v
