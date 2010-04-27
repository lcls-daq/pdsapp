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
    parser.add_option("-p","--port",dest="port",type="int",default=10130,
                      help="connect to DAQ at PORT", metavar="PORT")

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
        
#
#  Configure Cycle 1
#
    data = DaqScan.DAQData()
    data.setevents(100)
    data.addcontrol(DaqScan.ControlPV('EXAMPLEPV1',0))
    data.addcontrol(DaqScan.ControlPV('EXAMPLEPV2',0))
    data.send(s)
#
#  Wait for the DAQ to declare 'enabled'
#
    result = DaqScan.DAQStatus(s)
#
#  Enable the EVR sequence
#

#
#  Wait for the DAQ to declare 'disabled'
#
    result = DaqScan.DAQStatus(s)
#
#  Disable the EVR sequence
#

#  Cycle 2
    data = DaqScan.DAQData()
    data.setevents(100)
    data.addcontrol(DaqScan.ControlPV('EXAMPLEPV1',1))
    data.addcontrol(DaqScan.ControlPV('EXAMPLEPV2',0))
    data.send(s)
    result = DaqScan.DAQStatus(s)
#  Enable the sequence
    result = DaqScan.DAQStatus(s)
#  Disable the sequence

#  Cycle 3
    data = DaqScan.DAQData()
    data.setevents(100)
    data.addcontrol(DaqScan.ControlPV('EXAMPLEPV1',1))
    data.addcontrol(DaqScan.ControlPV('EXAMPLEPV2',1))
    data.send(s)
    result = DaqScan.DAQStatus(s)
#  Enable the sequence
    result = DaqScan.DAQStatus(s)
#  Disable the sequence

    s.close()
