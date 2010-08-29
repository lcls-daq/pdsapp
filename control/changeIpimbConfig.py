#!/reg/g/pcds/package/python-2.5.2/bin/python
#

import ConfigDb
import Ipimb
import subprocess

if __name__ == "__main__":

    import sys

    if (len(sys.argv)<2):
        print 'Usage: '+sys.argv[0]+' [1pF 100pF 10nF]'
        exit(1)
        
#
#  Iterate through all device configurations
#
    db = ConfigDb.Db()
    db.set_path('../configdb/current')
    devices = db.devices()
    for d in devices:
        names = db.xtcname(d,'STD')
        for c in names:
#
#  Edit each Ipimb configuration
#
            if c[0]=='IpimbConfig':
                xtcname = db.path+'/xtc/IpimbConfig_v1/'+c[1]
                cfg = Ipimb.ConfigV1(xtcname)
#
#  Change gain range of all 4 channels
#
                for channel in range(4):
                    if   sys.argv[1]=='1pF':
                        cfg.setChargeAmpRange_1pF  (channel)
                    elif sys.argv[1]=='100pF':
                        cfg.setChargeAmpRange_100pF(channel)
                    elif sys.argv[1]=='10nF':
                        cfg.setChargeAmpRange_10nF (channel)
                print xtcname,cfg.data
                cfg.write(xtcname)
#
#  Update the database
#
    plist = ['../current/build/pdsapp/bin/i386-linux-opt/configdb','--update-keys',db.path]
    subprocess.Popen(plist)
