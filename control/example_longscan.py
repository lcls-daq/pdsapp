#!/reg/g/pcds/package/python-2.5.2/bin/python
#

import socket
import DaqScan

from optparse import OptionParser

if __name__ == "__main__":
    import sys

    parser = OptionParser()
    parser.add_option("-a","--address",dest="host",default='',
                      help="connect to DAQ at HOST", metavar="HOST")
    parser.add_option("-p","--port",dest="port",type="int",default=10149,
                      help="connect to DAQ at PORT", metavar="PORT")
    parser.add_option("-n","--cycles",dest="cycles",type="int",default=100,
                      help="run N cycles", metavar="N")
    parser.add_option("-e","--events",dest="events",type="int",default=105,
                      help="record N events/cycle", metavar="N")

    (options, args) = parser.parse_args()
        
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

    s.connect((options.host,options.port))

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

    for cycle in range(options.cycles):
        data = DaqScan.DAQData()
        data.setevents(options.events)
        data.addcontrol(DaqScan.ControlPV('EXAMPLEPV1',cycle))
        data.addcontrol(DaqScan.ControlPV('EXAMPLEPV2',100-cycle))

        print "Cycle ", cycle
        data.send(s)

        result = DaqScan.DAQStatus(s)  # wait for enabled , then enable the EVR sequence

        result = DaqScan.DAQStatus(s)  # wait for disabled, then disable the EVR sequence
            
    s.close()
