#!/pcds/package/python-2.5.2/bin/python
#

import os
import shutil
import glob

from CfgDevice import *

class CfgExpt:
    """Manipulates database within a particular directory tree"""
    def __init__(self,path):
        self.basedir = path
        self.table   = CfgTable(None)
        self.devices = []
  
    def is_valid(self):
        if (os.path.exists(self.basedir) and
            os.path.exists(self.basedir+"/db") and
            os.path.exists(self.basedir+"/desc") and
            os.path.exists(self.basedir+"/xtc") and
            os.path.exists(self.basedir+"/keys")):
            return 1
        return 0
        
    def create(self):
        os.mkdir(self.basedir)
        os.mkdir(self.basedir+"/db")
        os.mkdir(self.basedir+"/desc")
        os.mkdir(self.basedir+"/xtc")
        os.mkdir(self.basedir+"/keys")

    def read(self):
        self.table   = CfgTable(self.basedir+"/db/expt")
        self.devices = []
        try:
            f=open(self.basedir+"/db/devices",'r')
            try:
                for line in f:
                    args=line.split()
                    path = self.basedir+"/db/devices."+args[0]
                    self.devices.append(CfgDevice(path,args[0],args[1:]))
            except EOFError:
                f.close()
        except IOError:
            pass
        
    def write(self):
        self.table.write(self.basedir+"/db/expt")
        f=open(self.basedir+"/db/devices","w")
        for item in self.devices:
            line=item.name
            for src in item.src_list:
                line += "\t" + src
            line += "\n"
            f.write(line)
        f.close()
        for item in self.devices:
            item.table.write(self.basedir+"/db/devices."+item.name)

    def device(self,name):
        for item in self.devices:
            if item.name==name:
                return item
        return None

    def add_device(self,name,src_list):
        device=CfgDevice(None,name,src_list)
        self.devices.append(device)
        os.mkdir(device.keypath(self.basedir,""))
#        os.mkdir(self.data_path(name,""))
#        os.mkdir(self.desc_path(name,""))
        

    def data_path(self,device,type):
        return self.basedir+"/xtc/"+type
#        return self.basedir+"/xtc/"+device+"/"+type
    
    def desc_path(self,device,type):
        return self.basedir+"/desc/"+type
#        return self.basedir+"/desc/"+device+"/"+type
    
    def xtc_files(self,device,type):
        list = glob.glob(self.data_path(device,type)+"/*")
        return map(os.path.basename, list)

    def import_data(self,device,type,file,desc):
        base = os.path.basename(file)
        dst = self.data_path(device,type)+"/"+base
        if os.path.exists(dst):
            print dst + " already exists."
            print "Rename the source file and try again."
            return
        dir = os.path.dirname(dst)
        if not os.path.exists(dir):
            os.mkdir(dir)
        shutil.copyfile(file,dst)
        dst = self.desc_path(device,type)+"/"+base
        dir = os.path.dirname(dst)
        if not os.path.exists(dir):
            os.mkdir(dir)
        f=open(dst,'w')
        f.write(desc + "\n")
        f.close()

    def key_path(self,device,key):
        return self.basedir+"/xtc/"+device+"/"+key
    
    def validate_key(self,entry):
        changed=0
        for item in entry.entries:
            changed+=self.device(item[0]).validate_key(item[1],self.basedir)

        #  Need to check that the top key points to the (now valid) device keys
        keypath = self.basedir+"/keys/"+entry.key
        invalid=len(entry.entries)
        if os.path.exists(keypath):
            for item in entry.entries:
                device=self.device(item[0])
                device_path="../"+item[0]+"/"+device.table.get_top_entry(item[1]).key
                invalid_device=len(device.src_list)
                for src in device.src_list:
                    srcpath=keypath+"/"+src
                    if (os.path.exists(srcpath) and
                        os.path.islink(srcpath) and
                        os.readlink(srcpath)==device_path):
                        invalid_device-=1
                if not invalid_device:
                    invalid-=1

        if invalid:
            entry.key="%08x" % len(glob.glob(self.basedir+"/keys/[0-9]*"))
            keypath=self.basedir+"/keys/"+entry.key
            os.mkdir(keypath)
            for item in entry.entries:
                device=self.device(item[0])
                device_path="../"+item[0]+"/"+device.table.get_top_entry(item[1]).key
                for src in device.src_list:
                    srcpath=keypath+"/"+src
                    os.symlink(device_path,srcpath)
            self.table.set_top_entry(entry)
        return [entry.key,invalid]
        
    def update_keys(self):
        for item in self.table.entries:
            [key,invalid]=self.validate_key(item)
            if invalid:
                print "Assigned new key " + key + " to " + item.name
                
    def dump(self):
        print "\nCfgExpt " + self.basedir
        self.table.dump(self.basedir+"/db/expt")
        print "\n" + self.basedir + "/db/devices"
        for item in self.devices:
            line=item.name
            for src in item.src_list:
                line += "\t" + src
            print line
        for item in self.devices:
            item.table.dump(self.basedir+"/db/devices."+item.name)

        
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
