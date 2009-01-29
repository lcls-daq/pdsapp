#!/pcds/package/python-2.5.2/bin/python

Class ConfigDb:
    def __init__(self):
        self.path = 'configdb'
    def keys(self):
        glob.glob(self.path+'/[0-9]*')
    def nodes(self,key):
        glob.glob(self.path+'/'+key+'/[0-9]*')

#if __name__ == "__main__":
#    import sys
#    fib(int(sys.argv[1]))
