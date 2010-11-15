#!/reg/g/pcds/package/python-2.5.2/bin/python
#

import socket
import DaqScan
import ConfigDb
import Evr

from optparse import OptionParser

#
#  Generate a configuration with incremented delays for each cycle
#
def replaceEvrConfig(cdb, oldkey, newkey, cycles):

    oldxtc = cdb.xtcpath(oldkey,Evr.DetInfo,Evr.TypeId)
    evr = Evr.ConfigV4()
    evr.read(oldxtc)

    newxtc = cdb.remove_xtc(newkey,Evr.DetInfo,Evr.TypeId)
    
    delay_offset = 0
    delay_step   = 100.e-9
    f = open(newxtc,'w')
    for cycle in range(cycles):
        evr.pulses[0].delay = int(delay_offset + cycle*delay_step*119e6)
        evr.write(f)
    f.close()

#
#  Generate a configuration with a changed exposure time
#
def replacePrincetonConfig(cdb, oldkey, newkey, exposure_time):

    oldxtc = cdb.xtcpath(oldkey,Princeton.DetInfo(ConfigDb.XppEndstation),Princeton.TypeId)
    prncton = Princeton.ConfigV1(oldxtc)

    newxtc = cdb.remove_xtc(newkey,Princeton.DetInfo(ConfigDb.XppEndstation),Princeton.TypeId)
    
    prncton.exposureTime = exposure_time
    prncton.write(f)
    f.close()

    
if __name__ == "__main__":
    import sys

    parser = OptionParser()
    parser.add_option("-a","--address",dest="host",default='xpp-daq',
                      help="connect to DAQ at HOST", metavar="HOST")
    parser.add_option("-p","--port",dest="port",type="int",default=10133,
                      help="connect to DAQ at PORT", metavar="PORT")
    parser.add_option("-n","--cycles",dest="cycles",type="int",default=100,
                      help="run N cycles", metavar="N")
    parser.add_option("-e","--events",dest="events",type="int",default=105,
                      help="record N events/cycle", metavar="N")

    (options, args) = parser.parse_args()
        
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

    s.connect((options.host,options.port))

#
#  First, get the current configuration key in use
#
    cdb = ConfigDb.Db()
    cdb.recv_path(s)
    key = DaqScan.DAQKey(s)

#
#  Generate a new key with different EVR pulse delays for each cycle
#
    newkey = cdb.copy_key(key.value)
    print 'Generated key ',newkey

    replaceEvrConfig      (cdb, key.value, newkey, options.cycles)
#    replacePrincetonConfig(cdb, key.value, newkey, 2.5)
    
#
#  Inform the DAQ to use the new key
#
    key.set(newkey)

#
#  Send the structure the first time to put the control variables
#    in the file header
#
    data = DaqScan.DAQData()
    data.setevents(0)
    data.addcontrol(DaqScan.ControlPV('EXAMPLEPV1',0))
    data.addcontrol(DaqScan.ControlPV('EXAMPLEPV2',0))
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

    for cycle in range(options.cycles):
        data = DaqScan.DAQData()
        data.setevents(options.events)
        data.addcontrol(DaqScan.ControlPV('EXAMPLEPV1',cycle))
        data.addcontrol(DaqScan.ControlPV('EXAMPLEPV2',100-cycle))

        print "Cycle ", cycle
        data.send(s)

        result = DaqScan.DAQStatus(s)  # wait for enabled , then enable the EVR sequence

        result = DaqScan.DAQStatus(s)  # wait for disabled, then disable the EVR sequence

#
#  Wait for the user to declare 'done'
#    Saving monitoring displays for example
#
    ready = raw_input('--Hit Enter when Done-->')
            
    s.close()
