import pycdb

def evolve(db,fname):
    y = db.get(typeid=0x00050008,src=0x100,file=fname)[0]
    v = db.evolve(6,y)
    v.set(y.get())
    db.set(file=fname,xtc=v)

db = pycdb.Db("/reg/g/pcds/dist/pds/cxi/configdb/current")
evolve(db,'beam_shutter_v5.xtc')
evolve(db,'beam_v5.xtc')
evolve(db,'twoevr_nobeam_v5.xtc')
       
