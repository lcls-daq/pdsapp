#!/usr/bin/python
##!/pcds/package/python-2.5.2/bin/python
#

import os

class CfgTableEntry:
    def __init__(self,args):
        self.name    = args[0]   # name of the configuration
        self.key     = args[1]   # key associated with the configuration
        self.entries = args[2]   # list of configuration components

    def set_entry(self,entry):
        entries=[]
        for item in self.entries:
            if not item[0]==entry[0]:
                entries.append(item)
        entries.append(entry)
        self.entries=entries

            
class CfgTable:
    def __init__(self,path):
        self.entries = []
        if path:
            top = self.read_table(path)
            for item in top:
                table = self.read_table(path + "." + item[0])
                self.entries.append(CfgTableEntry([item[0],item[1],table]))

    def read_table(self,path):
        list=[]
        try:
            f=open(path,'r')
            try:
                for line in f:
                    list.append(line.rstrip("\n").split("\t"))
            except EOFError:
                f.close()
        except IOError:
            pass
        list.sort()
        return list

    def write(self,path):
        top = []
        for item in self.entries:
            top.append([item.name,item.key])
        self.write_table(top,path)

        for item in self.entries:
            self.write_table(item.entries,path + "." + item.name)
        
    def write_table(self,table,path):
        f=open(path,'w')
        for item in table:
            f.write(item[0] + '\t' + item[1] + '\n')
        f.close()

    #  Return a copied list of strings of the names of each entry
    def get_top_names(self):
        list = []
        for item in self.entries:
            list.append(item.name)
        return list

    #  Return the CfgTableEntry matching <name>
    def get_top_entry(self,name):
        for item in self.entries:
            if item.name==name:
                return item
        return None
    
    def set_top_entry(self,entry):
        entries=[]
        for item in self.entries:
            if item.name==entry.name:
                entries.append(entry)
            else:
                entries.append(item)
        self.entries=entries
        
    def new_top_entry(self,name):
        self.entries.append(CfgTableEntry([name,"Unassigned",[]]))

    def copy_top_entry(self,dst,src):
        old = self.get_top_entry(src)
        self.entries.append(CfgTableEntry([dst,"Unassigned",old.entries]))

    def set_entry(self,top,entry):
        old = self.get_top_entry(top)
        if old:
            old.set_entry(entry)
        else:
            old=CfgTableEntry([top,"Unassigned",entry])
        self.set_top_entry(old)

    def dump(self,path):
        print "\nCfgTable " + path
        for item in self.entries:
            print item.name + "\t" + item.key
        for item in self.entries:
            print path + "." + item.name
            print item.entries
        
