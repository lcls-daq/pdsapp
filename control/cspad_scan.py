#!/reg/g/pcds/package/python-2.5.2/bin/python
#

import socket
import DaqScan
import ConfigDb
import Cspad

from optparse import OptionParser

if __name__ == "__main__":
    import sys

    parser = OptionParser()
    parser.add_option("-a","--address",dest="host",default='xpp-daq',
                      help="connect to DAQ at HOST", metavar="HOST")
    parser.add_option("-p","--port",dest="port",type="int",default=10133,
                      help="connect to DAQ at PORT", metavar="PORT")
    parser.add_option("-P","--parameter",dest="parameter",type="string",
                      help="cspad parameter to scan {\'runDelay\',\'intTime\'}", metavar="PARAMETER")
    parser.add_option("-r","--range",dest="range",type="int",nargs=2,default=[2,2],
                      help="parameter range", metavar="lo,hi")
    parser.add_option("-n","--steps",dest="steps",type="int",default=20,
                      help="run N parameter steps", metavar="N")
    parser.add_option("-e","--events",dest="events",type="int",default=105,
                      help="record N events/cycle", metavar="N")
    
    (options, args) = parser.parse_args()
        
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

    s.connect((options.host,options.port))

    print 'host', options.host
    print 'port', options.port
    print 'parameter', options.parameter
    print 'range', options.range
    print 'steps', options.steps
    print 'events', options.events
    
#
#  First, get the current configuration key in use and set the value to be used
#
    cdb = ConfigDb.Db()
    cdb.recv_path(s)
    key = DaqScan.DAQKey(s)

#
#  Generate a new key with different Cspad and EVR configuration for each cycle
#
    newkey = cdb.copy_key(key.value)
    print 'Generated key ',newkey

    cspad = Cspad.ConfigV2()
    cspad.read(cdb.xtcpath(key.value,Cspad.DetInfo,Cspad.TypeId))

    newxtc = cdb.remove_xtc(newkey,Cspad.DetInfo,Cspad.TypeId)

    f = open(newxtc,'w')
    value = options.range[0]
    step  = (options.range[1]-options.range[0])/options.steps
    for cycle in range(options.steps+1):
        if options.parameter=='runDelay':
            cspad.runDelay = value
        elif options.parameter=='intTime':
            for q in range(4):
                cspad.quads[q].intTime=value
        cspad.write(f)
        value = value + step
    f.close()

#
#  Could scan EVR simultaneously
#
#    evr   = Evr  .ConfigV4().read(cdb.xtcpath(key.value,Evr  .DetInfo,Evr  .TypeId))
#    newxtc = cdb.remove_xtc(newkey,Evr.DetInfo,Evr.TypeId)

    key.set(newkey)

#
#  Send the structure the first time to put the control variables
#    in the file header
#
    data = DaqScan.DAQData()
    data.setevents(0)
    data.addcontrol(DaqScan.ControlPV(options.parameter,0))
    data.send(s)
#
#  Wait for the DAQ to declare 'configured'
#
    result = DaqScan.DAQStatus(s)
    print "Configured."

#
#  Wait for the user to declare 'ready'
#    Setting up monitoring displays for example
#  
    ready = raw_input('--Hit Enter when Ready-->')

    value = options.range[0]
    step  = (options.range[1]-options.range[0])/options.steps
    for cycle in range(options.steps+1):
        data = DaqScan.DAQData()
        data.setevents(options.events)
        data.addcontrol(DaqScan.ControlPV(options.parameter,value))

        print "Cycle ", cycle
        data.send(s)

        result = DaqScan.DAQStatus(s)  # wait for enabled , then enable the EVR sequence

        result = DaqScan.DAQStatus(s)  # wait for disabled, then disable the EVR sequence

        value = value + step
        
#
#  Wait for the user to declare 'done'
#    Saving monitoring displays for example
#
    ready = raw_input('--Hit Enter when Done-->')

    s.close()

    
