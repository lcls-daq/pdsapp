#!/reg/g/pcds/package/python-2.5.2/bin/python
#

import pydaq
import pycdb

from optparse import OptionParser

if __name__ == "__main__":
    import sys

    parser = OptionParser()
    parser.add_option("-a","--address",dest="host",default='localhost',
                      help="connect to DAQ at HOST", metavar="HOST")
    parser.add_option("-p","--platform",dest="platform",type="int",default=3,
                      help="connect to DAQ at PLATFORM", metavar="PLATFORM")
    parser.add_option("-D","--detector",dest="detector",type="int",default=0x20000a00,
                      help="detector ID to scan",metavar="ID")
    parser.add_option("-P","--parameter",dest="parameter",type="string",
                      help="cspad parameter to scan {\'runDelay\',\'intTime\'}", metavar="PARAMETER")
    parser.add_option("-s","--start",dest="start",type="int",nargs=1,default=[2],
                      help="parameter start", metavar="START")
    parser.add_option("-m","--multiplier",dest="multiplier",type="float",nargs=1,default=2.0,
                      help="parameter multiplier, (float)", metavar="MULT")
    parser.add_option("-n","--steps",dest="steps",type="int",default=20,
                      help="run N parameter steps", metavar="N")
    parser.add_option("-e","--events",dest="events",type="int",default=105,
                      help="record N events/cycle", metavar="N")
    
    (options, args) = parser.parse_args()
    daq = pydaq.Control(options.host,options.platform)
    daq.connect()
    
    print 'host', options.host
    print 'platform', options.platform
    print 'parameter', options.parameter
    print 'start', options.start
    print 'steps', options.steps
    print 'multiplier', options.multiplier
    print 'events', options.events
    
#
#  First, get the current configuration key in use and set the value to be used
#
    
    cdb = pycdb.Db(daq.dbpath())
    key = daq.dbkey()

#
#  Generate a new key with different Cspad and EVR configuration for each cycle
#
    newkey = cdb.clone(key)
    print 'Generated key ',newkey

    xtc = cdb.get(key=key,src=options.detector,typeid=0x0004001d)[0]
    cspad = xtc.get(0)
    value = options.start
    for cycle in range(options.steps+1):
        if options.parameter=='runDelay':
            cspad['runDelay'] = value
        elif options.parameter=='intTime':
            for q in range(4):
                cspad['quads'][q]['intTime']=value
        xtc.set(cspad,cycle)
	value = int(value * options.multiplier)
    cdb.substitute(newkey,xtc)
    print 'done'
#
#  Could scan EVR simultaneously
#
#    evr   = Evr  .ConfigV4().read(cdb.xtcpath(key.value,Evr  .DetInfo,Evr  .TypeId))
#    newxtc = cdb.remove_xtc(newkey,Evr.DetInfo,Evr.TypeId)

#
#  Send the structure the first time to put the control variables
#    in the file header
#
    daq.configure(key=newkey,
                  events=options.events,
                  controls=[(options.parameter,options.start)])
    print "Configured."

#
#  Wait for the user to declare 'ready'
#    Setting up monitoring displays for example
#  
    ready = raw_input('--Hit Enter when Ready-->')

    value = options.start
    for cycle in range(options.steps):

        print "Cycle", cycle, "-", options.parameter, "=", value
        daq.begin(controls=[(options.parameter,value)])
        # wait for enabled , then enable the EVR sequence

        # wait for disabled, then disable the EVR sequence
        daq.end()  

	value = int(value * options.multiplier)
        
#
#  Wait for the user to declare 'done'
#    Saving monitoring displays for example
#
    ready = raw_input('--Hit Enter when Done-->')


    
