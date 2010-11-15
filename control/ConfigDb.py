#!/reg/g/pcds/package/python-2.5.2/bin/python
#

import glob
import os
import struct
from stat import *

from distutils import dir_util

AmoEndstation = 23
SxrEndstation = 12
XppEndstation = 22
CxiEndstation = 24
XcsEndstation = 25
MecEndstation = 26

#
#  class to inspect configuration database
#

class Db:
    def __init__(self):
        self.path = ''

    def set_path(self,path):
        self.path = path
        
    def recv_path(self,s):
        len = struct.unpack('<i',s.recv(4))[0]
        self.path = s.recv(len).rsplit('/',1)[0]
            
    def xtcname(self,device,alias):
        try:
            f = open(self.path+'/db/devices.'+device+'.'+alias,'r')
        except IOError:
            return []
        t = []
        for line in f:
            t += [line.split()]
        f.close()
        return t
        
    def devices(self):
        t = []
        f = open(self.path+'/db/devices','r')
        for line in f:
            t += [line.split()[0]]
        f.close()
        return t
    
    def keypath(self,key):
        return self.path+'/keys/'+"%08x" % key

    def next_key(self):
        return len(glob.glob(self.path+"/keys/[0-9]*"))

    def copy_key(self,oldkey):
        newkey = self.next_key()
        olddir = self.keypath(oldkey)
        newdir = self.keypath(newkey)
        print 'olddir='+olddir+', newdir='+newdir
        dir_util.copy_tree(olddir,newdir,preserve_symlinks=1)
        return newkey

    def xtcpath(self,key,device,type):
        return self.keypath(key)+'/'+device+'/'+type
    
    def remove_xtc(self,key,device,type):
        devdir = self.keypath(key)+'/'+device
        tgtpath = os.path.join(os.path.dirname(devdir),os.readlink(devdir))
        os.unlink(devdir)
        os.mkdir(devdir)
        dir_util.copy_tree(tgtpath,devdir,preserve_symlinks=1)
        xtcpath = devdir+'/'+type
        try:
            os.unlink(xtcpath)
        except OSError:
            pass
        return xtcpath

        
