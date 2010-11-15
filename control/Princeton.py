#!/reg/g/pcds/package/python-2.5.2/bin/python
#

import struct

TypeId  = '00040008'

def DetInfo(detector):
    return "%02x000600" % detector

class ConfigV1:
    def __init__(self,name):
        f = open(name,'r')
        (self.width, self.height, \
         self.orgX, self.orgY, \
         self.binX, self.binY, \
         self.exposureTime, \
         self.coolingTemp, \
         self.readoutSpeedIndex, \
         self.readoutEventCode, \
         self.delayMode) \
         = struct.unpack(self.fmt(),f.read(struct.calcsize(self.fmt())))
        f.close()

    def write(self,name):
        f = open(name,'w')
        f.write(struct.pack(self.fmt(),
                            self.width, self.height,
                            self.orgX, self.orgY,
                            self.binX, self.binY,
                            self.exposureTime,
                            self.coolingTemp,
                            self.readoutSpeedIndex,
                            self.readoutEventCode,
                            self.delayMode))
        f.close()
        
    def fmt(self):
        return '<IIIffIHH'

