import pycdb

def evolve(db,fname):
    y = db.get(typeid=0x00050008,src=0x100,file=fname)[0]
    v = x.evolve(6,y)
    v.set(y.get())
    x.set(file=fname,xtc=v)

db = pycdb.Db("/reg/lab2/home/tstopr/configdb/weaver")
evolve(db,'cspad_v3.xtc')
evolve(db,'live_scan.xtc')
evolve(db,'jack_newer.xtc')
       
