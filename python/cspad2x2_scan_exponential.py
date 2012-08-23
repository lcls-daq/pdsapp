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
    parser.add_option("-D","--detector",dest="detector",type="int",default=0x19000d02,
                      help="detector ID to scan",metavar="ID")
    parser.add_option("-t","--typeID",dest="typeID",type="int",default=0x1002b,
                      help="type ID to generate",metavar="TYPEID")
    parser.add_option("-P","--parameter",dest="parameter",type="string",
                      help="cspad2x2 quad parameter to scan", metavar="PARAMETER")
    parser.add_option("-s","--start",dest="start",type="int",nargs=1,default=[2],
                      help="parameter start", metavar="START")
    parser.add_option("-m","--multiplier",dest="multiplier",type="float",nargs=1,default=2.0,
                      help="parameter multiplier, (float)", metavar="MULT")
    parser.add_option("-n","--steps",dest="steps",type="int",default=20,
                      help="run N parameter steps", metavar="N")
    parser.add_option("-e","--events",dest="events",type="int",default=105,
                      help="record N events/cycle", metavar="N")
    parser.add_option("-l","--limit",dest="limit",type="int",default=99,
                      help="limit number of configs to less than number of steps", metavar="N")
    (options, args) = parser.parse_args()
    
    print 'host', options.host
    print 'platform', options.platform
    print 'parameter', options.parameter
    print 'start', options.start
    print 'steps', options.steps
    print 'multiplier', options.multiplier
    print 'events', options.events
    print 'detector', hex(options.detector)
    print 'typeID', hex(options.typeID)
    if options.steps<options.limit : options.limit = options.steps
    else : print 'Warning, range will be covered in', options.limit, \
         'but will still do', options.steps, 'steps with wrapping'
    daq = pydaq.Control(options.host,options.platform)
    daq.connect()

#
#  First, get the current configuration key in use and set the value to be used
#
    
    cdb = pycdb.Db(daq.dbpath())
    key = daq.dbkey()
    print 'Retrieved key ',hex(key)

#
#  Generate a new key with different Cspad and EVR configuration for each cycle
#
    newkey = cdb.clone(key)
    print 'Generated key ',hex(newkey)

    xtc = cdb.get(key=key,src=options.detector,typeid=options.typeID)[0]
    print 'xtc is', xtc
    cspad = xtc.get(0)
    foundParameter = 'FALSE'
    for member in cspad['quad'][0] :
        if member == options.parameter :
            print 'Found the', options.parameter, 'parameter'
            foundParameter = 'TRUE'
    if foundParameter == 'FALSE' :
        print 'Parameter', options.parameter, 'not found!'
        print '    Allowed parameters : current values'
        for member in cspad['quad'][0] :
            if member!='gain' and member!='pots' and not member.endswith('Select') :
		print '        ', member, ':',   cspad['quad'][0][member]
#    print 'cspad inactiveRunMode =', cspad['inactiveRunMode'], 'activeRunMode =', cspad['activeRunMode'], 'payloadSize =', cspad['payloadSize']
    if foundParameter == 'TRUE' :
        print 'Now we will start scanning ...'
        value = options.start
        for cycle in range(options.limit+1):
            cspad['quad'][0][options.parameter]=value
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

        for cycle in range(options.steps):
            if cycle%(options.limit+1) == 0 : value = options.start
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


    
