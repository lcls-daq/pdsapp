#!/pcds/package/python-2.5.2/bin/python
#

import os
import shutil
import glob

from CfgDevice import *
from CfgExpt import *
        
def print_help(arg):
    print "Transactions:"
    print "  Create device with source ids"
    print arg + " --create-device <DEV> <Src1> .. <SrcN>"
    print "  Create an alias entry for device"
    print arg + " --create-device-alias <DEV> <ALIAS>"
    print "  Copy an alias entry for device"
    print arg + " --copy-device-alias <DEV> <NEW_ALIAS> <OLD_ALIAS>"
    print "  Import configuration data for device"
    print arg + " --import-device-data <DEV> <Type> <File> ""Description"""
    print "  Assign configuration data to an alias entry for device"
    print arg + " --assign-device-alias <DEV> <ALIAS> <Type> <File>"
    print ""
    print "  Create a expt alias entry"
    print arg + " --create-expt-alias <ALIAS>"
    print "  Copy a expt alias entry"
    print arg + " --copy-expt-alias <NEW_ALIAS> <OLD_ALIAS>"
    print "  Assign a device alias entry to a expt alias entry"
    print arg + " --assign-expt-alias <ALIAS> <DEV> <DEV_ALIAS>"
    print ""
    print "  Create the database"
    print arg + "--create"
    print ""
    print "DB Management"
    print "  Update keys"
    print arg + " --update-keys"
        
if __name__ == "__main__":
    import sys

    nargs = len(sys.argv)
    if sys.argv[1] == "--help":    
        print_help(sys.argv[0])
        sys.exit(0)


    if nargs<3:
        print "Too few arguments"
        print_help(sys.argv[0])
        sys.exit(0)

    cmd = sys.argv[1]
    db  = CfgExpt(sys.argv[2])

#    print "\n==BEFORE=="
#    db.dump()

    if not db.is_valid():
        if cmd == "--create":
            db.create()
        else:
            print "No valid database found at " + sys.argv[2]
            print "Exiting."
            sys.exit(1)
    else:
        db.read()
        
        if   cmd == "--update-keys":
            db.update_keys()
        else:

            dev = sys.argv[3]

            if   cmd == "--create-device":
                db.add_device(dev,sys.argv[4:])
            elif cmd == "--create-device-alias":
                db.device(dev).table.new_top_entry(sys.argv[4])
            elif cmd == "--copy-device-alias":
                db.device(dev).table.copy_top_entry(*sys.argv[4:6])
            elif cmd == "--import-device-data":
                db.import_data(*sys.argv[3:7])
            elif cmd == "--assign-device-alias":
                db.device(dev).table.set_entry(sys.argv[4],sys.argv[5:7])
            elif cmd == "--create-expt-alias":
                db.table.new_top_entry(sys.argv[3])
            elif cmd == "--copy-expt-alias":
                db.table.copy_top_entry(*sys.argv[3:5])
            elif cmd == "--assign-expt-alias":
                db.table.set_entry(sys.argv[3],sys.argv[4:6])
            else:
                print "unknown command %s, try --help" % (cmd)

#    print "\n==AFTER=="
#    db.dump()
    db.write()
