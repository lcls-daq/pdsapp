#!/reg/g/pcds/package/python-2.5.2/bin/python
#

import struct

DetInfo = '14000a00'

TypeId  = '0001001d'

class DigitalPotsCfg:
    def __init__(self):
        self.pots = []

    def read(self,f):
        self.pots = []
        for i in range(80):
            self.pots.append(struct.unpack('<B',f.read(struct.calcsize('<B')))[0])
        return self
    
    def write(self,f):
        for i in range(80):
            f.write(struct.pack('<B',self.pots[i]))

class ReadOnlyCfg:
    def __init__(self):
        self.shiftTest = 0
        self.version   = 0

    def read(self,f):
        (self.shiftTest, self.version) \
         = struct.unpack(self.fmt(),f.read(struct.calcsize(self.fmt())))
        return self

    def write(self,f):
        f.write(struct.pack(self.fmt(),self.shiftTest,self.version))

    def fmt(self):
        return '<2I'
    
class GainMapCfg:
    def __init__(self):
        self.gainMap = []

    def read(self,f):
        self.gainMap = []
        for i in range(185*194):
            self.gainMap.append(struct.unpack('<H',f.read(struct.calcsize('<H')))[0])
        return self
    
    def write(self,f):
        for i in range(185*194):
            f.write(struct.pack('<H',self.gainMap[i]))

    def clear(self):
        self.gainMap = []
        for i in range(185*194):
            self.gainMap.append(0)

    def set(self):
        self.gainMap = []
        for i in range(185*194):
            self.gainMap.append(0xFFFF)
        
class QuadV1:
    def __init__(self):
        self.shiftSelect = []
        self.edgeSelect  = []
        self.readClkSet  = 0
        self.readClkHold = 0
        self.dataMode    = 0
        self.prstSel     = 0
        self.acqDelay    = 0
        self.intTime     = 0
        self.digDelay    = 0
        self.ampIdle     = 0
        self.injTotal    = 0
        self.rowColShiftPer = 0
        self.readOnly    = ReadOnlyCfg()
        self.digitalPots = DigitalPotsCfg()
        self.gainMap     = GainMapCfg()
        
    def read(self,f):
        self.shiftSelect = []
        for i in range(4):
            self.shiftSelect.append(struct.unpack('<I',f.read(struct.calcsize('<I')))[0])

        self.edgeSelect = []
        for i in range(4):
            self.edgeSelect.append(struct.unpack('<I',f.read(struct.calcsize('<I')))[0])
            
        (self.readClkSet,self.readClkHold, \
         self.dataMode,self.prstSel, \
         self.acqDelay,self.intTime,self.digDelay, \
         self.ampIdle, self.injTotal, self.rowColShiftPer) \
         = struct.unpack(self.fmt(),f.read(struct.calcsize(self.fmt())))

        self.readOnly    = ReadOnlyCfg   ().read(f)
        self.digitalPots = DigitalPotsCfg().read(f)
        self.gainMap     = GainMapCfg    ().read(f)

        return self

    def write(self,f):
        for i in range(4):
            f.write(struct.pack('<I',self.shiftSelect[i]))

        for i in range(4):
            f.write(struct.pack('<I',self.edgeSelect[i]))

        f.write(struct.pack(self.fmt(),
                            self.readClkSet,
                            self.readClkHold,
                            self.dataMode,
                            self.prstSel,
                            self.acqDelay,
                            self.intTime,
                            self.digDelay,
                            self.ampIdle,
                            self.injTotal,
                            self.rowColShiftPer))

        self.readOnly   .write(f)
        self.digitalPots.write(f)
        self.gainMap    .write(f)

    def fmt(self):
        return '<10I'

class ConfigV1:
    def __init__(self):
        self.concentratorVersion = 0
        self.runDelay = 0
        self.eventCode = 0
        self.inactiveRunMode = 0
        self.activeRunMode = 0
        self.testDataIndex = 0
        self.payloadPerQuad = 0
        self.badAsicMask0 = 0
        self.badAsicMask1 = 0
        self.asicMask = 0
        self.quadMask = 0
        self.quads = []

    def read(self,name):
        f = open(name,'r')

        (self.concentratorVersion, \
         self.runDelay, self.eventCode, self.inactiveRunMode, self.activeRunMode, \
         self.testDataIndex, self.payloadPerQuad, self.badAsicMask0, self.badAsicMask1, \
         self.asicMask, self.quadMask) \
        = struct.unpack(self.fmt(),f.read(struct.calcsize(self.fmt())))

        print self.eventCode, self.asicMask, self.quadMask
        
        self.quads = []
        for i in range(4):
            self.quads.append(QuadV1().read(f))
            
        f.close()

    def write(self,f):
        f.write(struct.pack(self.fmt(),
                            self.concentratorVersion,
                            self.runDelay, self.eventCode, self.inactiveRunMode, self.activeRunMode,
                            self.testDataIndex, self.payloadPerQuad, self.badAsicMask0, self.badAsicMask1,
                            self.asicMask, self.quadMask))

        for i in range(4):
            self.quads[i].write(f)
        
    def fmt(self):
        return '<11I'


class ConfigV2:
    def __init__(self):
        self.concentratorVersion = 0
        self.runDelay = 0
        self.eventCode = 0
        self.inactiveRunMode = 0
        self.activeRunMode = 0
        self.testDataIndex = 0
        self.payloadPerQuad = 0
        self.badAsicMask0 = 0
        self.badAsicMask1 = 0
        self.asicMask = 0
        self.quadMask = 0
        self.roiMask = 0
        self.quads = []

    def read(self,name):
        f = open(name,'r')

        (self.concentratorVersion, \
         self.runDelay, self.eventCode, self.inactiveRunMode, self.activeRunMode, \
         self.testDataIndex, self.payloadPerQuad, self.badAsicMask0, self.badAsicMask1, \
         self.asicMask, self.quadMask, self.roiMask) \
        = struct.unpack(self.fmt(),f.read(struct.calcsize(self.fmt())))

        print self.eventCode, self.asicMask, self.quadMask, self.roiMask
        
        self.quads = []
        for i in range(4):
            self.quads.append(QuadV1().read(f))
            
        f.close()

    def write(self,f):
        f.write(struct.pack(self.fmt(),
                            self.concentratorVersion,
                            self.runDelay, self.eventCode, self.inactiveRunMode, self.activeRunMode,
                            self.testDataIndex, self.payloadPerQuad, self.badAsicMask0, self.badAsicMask1,
                            self.asicMask, self.quadMask, self.roiMask))

        for i in range(4):
            self.quads[i].write(f)
        
    def fmt(self):
        return '<12I'
