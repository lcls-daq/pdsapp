import pycdb

x = pycdb.Db("/reg/neh/home/weaver/configdb/xpp")
y = x.get("BEAM",1,0x0f010800)[1]
y.get_base()
y.get_scale()
y.set_scale([2,1,1])
y.get_scale()

z = x.clone(0x27)
x.substitute(z,y)

x.set(y)
x.commit()
