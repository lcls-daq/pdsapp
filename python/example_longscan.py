#!/reg/g/pcds/package/python-2.5.2/bin/python
#

import pydaq
import sys
import time

from optparse import OptionParser

if __name__ == "__main__":
    import sys

    parser = OptionParser()
    parser.add_option("-a","--address",dest="host",default='xpp-daq',
                      help="connect to DAQ at HOST", metavar="HOST")
    parser.add_option("-p","--platform",dest="platform",type="int",default=0,
                      help="connect to DAQ platform P", metavar="P")
    parser.add_option("-n","--cycles",dest="cycles",type="int",default=100,
                      help="run N cycles", metavar="N")
    parser.add_option("-e","--events",dest="events",type="int",default=105,
                      help="record N events/cycle", metavar="N")
    parser.add_option("-q","--qbeam",dest="qbeam",type="float",default=-1.,
                      help="require qbeam > Q", metavar="Q")
    parser.add_option("-s","--sleep",dest="tsleep",type="float",default=0.,
                      help="sleep Q seconds between cycles", metavar="Q")
    parser.add_option("-w","--wait",dest="wait",default=False,
                      help="wait for key input before 1st cycle and after last cycle", metavar="W");

    (options, args) = parser.parse_args()
    daq = pydaq.Control(options.host,options.platform)

    if options.wait:
        ready = raw_input('--Hit Enter when Ready-->')

#
#  Send the structure the first time to put the control variables
#    in the file header
#

#    daq.configure(events=options.events,
#                  controls=[('EXAMPLEPV1',0),('EXAMPLEPV2',0)],
#                  monitors=[('BEAM:LCLS:ELEC:Q',options.qbeam,1.)])

    do_record = False
    
    daq.configure(record=do_record,
                  events=options.events,
                  controls=[('EXAMPLEPV1',0),('EXAMPLEPV2',0)],
                  labels=[('EXAMPLELABEL1',''),('EXAMPLELABEL2','')])

    print "Configured."

#
#  Wait for the user to declare 'ready'
#    Setting up monitoring displays for example
#  

    if options.wait:
        ready = raw_input('--Hit Enter when Ready-->')

    for cycle in range(options.cycles):
        print "Cycle ", cycle
        time.sleep(options.tsleep)
        daq.begin(controls=[('EXAMPLEPV1',cycle),('EXAMPLEPV2',100-cycle)],
                  labels=[('EXAMPLELABEL1','CYCLE%d'%cycle),('EXAMPLELABEL2','LCYCLE%d'%options.cycles)])
        # enable the EVR sequence, if necessary

        # wait for disabled, then disable the EVR sequence
        daq.end()
            
    if (do_record==True):
        print 'Recorded expt %d run %d' % (daq.experiment(),daq.runnumber())
        
#
#  Wait for the user to declare 'done'
#    Saving monitoring displays for example
#
    if options.wait:
        ready = raw_input('--Hit Enter when Done-->')
