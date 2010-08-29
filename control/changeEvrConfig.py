#!/reg/g/pcds/package/python-2.5.2/bin/python
#
#  Example script for changing the EVR configuration
#
#    This example take command line parameters for which configuration
#  alias to change {BEAM,NO_BEAM,...}, which pulse #, and the new delay
#  and width settings.
#

import ConfigDb
import Evr
import subprocess

from optparse import OptionParser

if __name__ == "__main__":

#    binpath = '../current/build/pdsapp/bin/i386-linux-opt/configdb'
    binpath = '/reg/neh/home/weaver/release/build/pdsapp/bin/i386-linux-opt/configdb'
    
    parser = OptionParser()
    parser.add_option("-b","--db",dest="dbpath",
                      default='/reg/g/pcds/dist/pds/xpp/configdb/current',
                      help="configuration db to edit", metavar="DBPATH")
    parser.add_option("-a","--alias",dest="alias",default='BEAM',
                      help="configuration alias to edit", metavar="ALIAS")
    parser.add_option("-p","--pulse",dest="pulse",type='int',default=0,
                      help="evr pulse to edit", metavar="PULSE")
    parser.add_option("-d","--delay",dest="delay",type='int',default=0,
                      help="delay (ns)", metavar="DELAY")
    parser.add_option("-w","--width",dest="width",type='int',default=100,
                      help="width (ns)", metavar="WIDTH")

    (options, args) = parser.parse_args()
    
#
#  Iterate through all device configurations
#
    db = ConfigDb.Db()
    db.set_path(options.dbpath)
    devices = db.devices()
    for d in devices:
        names = db.xtcname(d,options.alias)
        for c in names:
#
#  Edit the EVR configuration
#
            if c[0]=='EvrConfig':
                xtcname = db.path+'/xtc/EvrConfig_v4/'+c[1]
                cfg = Evr.ConfigV4()
                cfg.read(xtcname)
#
#  Change pulse settings
#
                cfg.pulses[options.pulse].delay = int(options.delay*Evr.EvrClk*1.e-9 + 0.5)
                cfg.pulses[options.pulse].width = int(options.width*Evr.EvrClk*1.e-9 + 0.5)

                f = open(xtcname,'w')
                cfg.write(f)
                f.close()
#
#  Update the database
#
    plist = [binpath,'--update-keys',db.path]
    subprocess.Popen(plist)
