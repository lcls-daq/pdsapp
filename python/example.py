import pycdb

def evr_evolve(db,fname):
    y = db.get(typeid=0x00050008,src=0x100,file=fname)[0]
    v = db.evolve(6,y)
    v.set(y.get())
    db.set(file=fname,xtc=v)

def cspad_evolve(db,fname):
    y = db.get(typeid=0x0003001d,src=0x14000a,file=fname)[0]
    print y
    v = db.evolve(4,y)
    v.set(y.get())
    print v.get()
    db.set(file=fname,xtc=v)
    
#db = pycdb.Db("/reg/g/pcds/dist/pds/cxi/configdb/current")
#evr_evolve(db,'beam_shutter_v5.xtc')
#evr_evolve(db,'beam_v5.xtc')
#evr_evolve(db,'twoevr_nobeam_v5.xtc')

db = pycdb.Db("/reg/g/pcds/dist/pds/xpp/configdb/current")
cspad_evolve(db,"cspadV3_01.xtc")
cspad_evolve(db,"test_V3_01.xtc")
