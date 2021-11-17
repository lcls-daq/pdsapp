from psana import *
ds = DataSource("foo-c00.xtc")
#for d in DetNames():
#    print d

wave8   = Detector('XrayTransportDiagnostic.0:Wave8.0')
qadc    = Detector('XrayTransportDiagnostic.1:Wave8.0')
qadc134 = Detector('XrayTransportDiagnostic.2:Wave8.0')
cam0    = Detector('XrayTransportDiagnostic.3:ControlsCamera.0')
cam1    = Detector('XrayTransportDiagnostic.3:ControlsCamera.1')
cam2    = Detector('XrayTransportDiagnostic.3:ControlsCamera.2')
cam3    = Detector('XrayTransportDiagnostic.3:ControlsCamera.3')
cam4    = Detector('XrayTransportDiagnostic.3:ControlsCamera.4')
wf0     = Detector('XrayTransportDiagnostic.4:Wave8.0')
wf1     = Detector('XrayTransportDiagnostic.4:Wave8.1')
wf2     = Detector('XrayTransportDiagnostic.4:Wave8.2')
wf3     = Detector('XrayTransportDiagnostic.4:Wave8.3')
wf4     = Detector('XrayTransportDiagnostic.4:Wave8.4')

def check_wave8(v, fid):
    if len(v) != 16:
        print "ERROR@%d: wave8 should have 16 arrays (actual %d)!" % (fid, len(v))
        return
    for i in range(16):
        size = 100 + 10 * i
        mask = 0x10000 if i < 8 else 0x100000000
        if v[i].shape != (size,):
            print "ERROR@%d: wave8 channel %d should be 1D with length %d (actual %s)." % (fid, offset,
                                                                                           size, v.shape)
            return
        for j in range(size):
            if v[i][j] != (fid + i + j) % mask:
                print "ERROR@%d: wave8 channel %d[%d] should be %g, not %g." % (fid, i, j, 
                                                                                (start + i + j) % mask,
                                                                                v[i][j])

def check_qadc(v, fid):
    if len(v) not in [1, 4]:
        print "ERROR@%d: qadc should have either 1 or 4 streams (actual %d)." % (fid, len(v))
        return
    streams = {}
    if len(v) == 1:
        streams[4] = v[0]
    else:
        for (n, s) in enumerate(v):
            streams[n] = s
    for (k, a) in streams.items():
        if a.shape != (1600,):
            print "ERROR@%d: qadc stream %d should be 1D with length 1600 (actual %s)." % (fid, k, a.shape)
            return
        for i in range(1600):
            sv = (((fid + i + k) % 0x1000) - 512.) / 2048.
            if a[i] != sv:
                print "ERROR@%d: qadc stream %d[%d] should be %g (actual %g)." % (fid, k, i, sv, a[i])
                return

def check_qadc134(v, fid):
    if len(v) not in [1, 2]:
        print "ERROR@%d: qadc134 should have either 1 or 2 streams (actual %d)." % (fid, len(v))
        return
    streams = {}
    if len(v) == 1:
        streams[2] = v[0]
    else:
        for (n, s) in enumerate(v):
            streams[n] = s
    for (k, a) in streams.items():
        if a.shape != (1600,):
            print "ERROR@%d: qadc134 stream %d should be 1D with length 1600 (actual %s)." % (fid, k, a.shape)
            return
        for i in range(1600):
            sv = (((fid + i + k) % 0x1000) - 2048.) / 4096. * 1.3
            if a[i] != sv:
                print "ERROR@%d: qadc134 stream %d[%d] should be %g (actual %g)." % (fid, k, i, sv, a[i])
                return

camsize  = [(300,400),(200,300),(500,500),(300,200),(400,300)]
camdepth = [8, 8, 12, 12, 16]

def check_cam(v, fid, offset):
    size = camsize[offset]
    depth = 1 << camdepth[offset]
    start = fid + offset
    if v.shape != size:
        print "ERROR@%d: wf%d should have shape %s (actual %s)." % (fid, offset, size, v.shape)
        return
    for i in range(size[0]):
        for j in range(size[1]):
            if v[i][j] != (start + i + j) % depth:
                print "ERROR@%d: cam%d[%d][%d] should be %g, not %g." % (fid, offset, i, j, 
                                                                         (start + i) % depth, v[i][j])

def check_wf(v, fid, offset):
    if len(v) != 1:
        print "ERROR@%d: wf%d should only have one array!" % (fid, offset)
        return
    v = v[0]
    size = 100 + 10 * offset
    start = fid + offset
    if v.shape != (size,):
        print "ERROR@%d: wf%d should be 1D with length %d (actual %s)." % (fid, offset, size, v.shape)
        return
    for i in range(size):
        if v[i] != (start + i) % 256:
            print "ERROR@%d: wf%d[%d] should be %g, not %g." % (fid, offset, i, (start + i) % 256, v[i])
    pass

for evt in ds.events():
   fid = evt.get(EventId).fiducials()
   print fid
   v=wave8(evt)
   if v is not None:
       check_wave8(v, fid)
   v=qadc(evt)
   if v is not None:
       check_qadc(v, fid)
   v=qadc134(evt)
   if v is not None:
       check_qadc134(v, fid)
   v=cam0.raw(evt)
   if v is not None:
       check_cam(v, fid, 0)
   v=cam1.raw(evt)
   if v is not None:
       check_cam(v, fid, 1)
   v=cam2.raw(evt)
   if v is not None:
       check_cam(v, fid, 2)
   v=cam3.raw(evt)
   if v is not None:
       check_cam(v, fid, 3)
   v=cam4.raw(evt)
   if v is not None:
       check_cam(v, fid, 4)
   v=wf0(evt)
   if v is not None:
       check_wf(v, fid, 0)
   v=wf1(evt)
   if v is not None:
       check_wf(v, fid, 1)
   v=wf2(evt)
   if v is not None:
       check_wf(v, fid, 2)
   v=wf3(evt)
   if v is not None:
       check_wf(v, fid, 3)
   v=wf4(evt)
   if v is not None:
       check_wf(v, fid, 4)
