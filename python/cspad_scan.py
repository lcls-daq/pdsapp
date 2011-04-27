#!/reg/g/pcds/package/python-2.5.2/bin/python
#

import pydaq
import pycdb

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
    parser.add_option("-l","--limit",dest="limit",type="int",default=10000,
                      help="limit number of configs to less than number of steps", metavar="N")
    
    (options, args) = parser.parse_args()
    daq = pydaq.Control(options.host,options.port)
    
    print 'host', options.host
    print 'port', options.port
    print 'parameter', options.parameter
    print 'range', options.range
    print 'steps', options.steps
    print 'events', options.events
    if options.steps<options.limit : options.limit = options.steps
    else : print 'Warning, range will be covered in', options.limit, \
         'but will still do', options.steps, 'steps with wrapping'
    
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

    xtc = cdb.get(key=key,typeid=0x0002001d)[0]
    cspad = xtc.get()
    extent = options.range[1]-options.range[0]
    for cycle in range(options.limit+1):
        value = ((cycle*extent)/options.limit) + options.range[0]
        if options.parameter=='runDelay':
            cspad['runDelay'] = value
        elif options.parameter=='intTime':
            for q in range(4):
                cspad['quads'][q]['intTime']=value
    xtc.set(cspad)
    cdb.substitute(newkey,xtc)

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
                  controls=[(options.parameter,options.range[0])])
    print "Configured."

#
#  Wait for the user to declare 'ready'
#    Setting up monitoring displays for example
#  
    ready = raw_input('--Hit Enter when Ready-->')

    lastStep = 0
    if options.limit==options.steps : lastStep = 1

    for cycle in range(options.steps+lastStep):
        value = (((cycle%(options.limit+1))*extent)/options.limit) + options.range[0]

        print "Cycle", cycle, "-", options.parameter, "=", value
        daq.begin(controls=[(options.parameter,value)])
        # wait for enabled , then enable the EVR sequence

        # wait for disabled, then disable the EVR sequence
        daq.end()  
        
#
#  Wait for the user to declare 'done'
#    Saving monitoring displays for example
#
    ready = raw_input('--Hit Enter when Done-->')


    
