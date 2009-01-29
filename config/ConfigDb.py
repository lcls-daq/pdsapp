#!/usr/bin/python
##!/pcds/package/python-2.5.2/bin/python
#

import glob
import os
import re
import shutil

def strippath(filename):
    """Strip path from filename"""
    return os.path.basename(filename)
#    return re.sub(r'(.*/)([^/]*$)',r'\2',filename)
    
class Db:
    """Manipulates database within a particular directory tree"""
    def __init__(self,dir):
        self.basedir = dir

    def key_dir(self,key):
        return self.basedir + '/' + key

    def node_path(self,key,node):
        return self.key_dir(key) + '/' + node

    def validate_key(self,key):
        if len(key) > 8:
            raise KeyError(key)
        return '00000000'[0:8-len(key)] + key

    def validate_node(self,node):
        if len(node) != 8:
            raise NodeError((node,'node must be 8-digis (hex)'))
#        if int(node[0:2]) > 4:
#            raise NodeError((node,'node level is invalid'))
        return "%08x" % int(node,16)

    def validate_type(self,type):
        return self.validate_key(type)
    
    def validate_file(self,file):
        afile = self.basedir + '/data/' + file
        if not os.path.exists(afile):
            raise FileError((afile,'path does not exist'))
        file = '../../data/' + file
        return file
    
    def list_keys(self):
        list=glob.glob(self.basedir + '/[0-9]*')
        return map(os.path.basename,list)

    def list_nodes(self,arg):
        key=self.validate_key(arg)
        list=glob.glob(self.key_dir(key) + '/[0-9]*')
        return map(os.path.basename,list)

    def list_types(self,key,node):
        key=self.validate_key(key)
        node=self.validate_node(node)
        list=glob.glob(self.node_path(key,node) + '/[0-9]*')
        return map(os.path.basename,list)

    def insert(self,args):
        key =self.validate_key (args[0])
        node=self.validate_node(args[1])
        tyid=self.validate_type(args[2])
        file=self.validate_file(args[3])
        if key not in self.list_keys():
            os.mkdir(self.key_dir(key))
        if node not in self.list_nodes(key):
            os.mkdir(self.node_path(key,node))
#        shutil.copyfile(file,self.node_path(key,node) + '/' + tyid)
        os.symlink(file,self.node_path(key,node) + '/' + tyid)
            
def print_help(arg):
    print "Usage: --list-keys"
    print "       --list-nodes <key>"
    print "       --list-types <key>"
    print "       --insert <key> <node> <type> <file>"
    print "       --help"
    print "  <key> is the hexadecimal configuration key number"
    print "  <node> is the hexadecimal device number"
    print "  <type> is the hexadecimal configuration data type number"
    print "  <file> is the configuration data file to be stored"
        
import sys

if len(sys.argv)<1 or sys.argv[1] == "--help":
    print_help(sys.argv[0])
    sys.exit(0)
    
db=Db("configdb/02/lab1")
if sys.argv[1] == "--list-keys":
    for key in db.list_keys():
        print key
elif sys.argv[1] == "--list-nodes":
    if len(sys.argv) > 2:
        for node in db.list_nodes(sys.argv[2]):
            print node
    else:
        print "--list-nodes missing <key> argument"
elif sys.argv[1] == "--list-types":
    if len(sys.argv) > 2:
        for node in db.list_nodes(sys.argv[2]):
            for type in db.list_types(sys.argv[2],node):
                print node + '/' + type
    else:
        print "--list-nodes missing <key> argument"
elif sys.argv[1] == "--insert":
    if len(sys.argv) > 5:
        db.insert(sys.argv[2:6])
    else:
        print "--insert: too few arguments"
else:
    print "unknown argument %s, try --help" % (sys.argv[1])

