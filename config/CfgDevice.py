#!/usr/bin/python
##!/pcds/package/python-2.5.2/bin/python
#

import os
import shutil
import glob
from CfgTable import *
from pds_defs import *

class CfgDevice:
    def __init__(self,path,name,src_list):
        self.name     = name
        self.src_list = src_list
        self.table    = CfgTable(path)

    def keypath(self,path,key):
        return path + "/keys/" + self.name + "/" + key

    def typepath(self,path,key,entry):
        return self.keypath(path,key)+"/"+pds_type_index(entry[0])

    def typelink(self,entry):
        return "../../../xtc/"+entry[0]+"/"+entry[1]
    
    """ Checks that the key link exists and is up to date """
    def validate_key(self,config,path):
        entry=self.table.get_top_entry(config)
        invalid=len(entry.entries)
        keypath=self.keypath(path,entry.key)
        if os.path.exists(keypath):
            for item in entry.entries:
                typepath=self.typepath(path,entry.key,item)
                if (os.path.exists(typepath) and
                    os.path.islink(typepath) and
                    os.readlink(typepath)==self.typelink(item)):
                        invalid-=1
        if invalid:
            entry.key="%08x" % len(glob.glob(self.keypath(path,"[0-9]*")))
            keypath=self.keypath(path,entry.key)
            os.mkdir(keypath)
            for item in entry.entries:
                os.symlink(self.typelink(item),self.typepath(path,entry.key,item))
            self.table.set_top_entry(entry)
        return invalid
