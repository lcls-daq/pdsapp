import pycdb

x = pycdb.Db("/reg/neh/home/weaver/configdb/xpp")
y = x.get(alias="BEAM",src=0x100)[1]
z = y.get()
z['pulses'][0]['polarity'] = 'Pos'
y.set(z)
x.set(alias="BEAM",y)

y = x.get(alias="BEAM",src=0x0f010800)[1]
y.get_base()
y.get_scale()
y.set_scale([2,1,1])
y.get_scale()

z = x.clone(0x27)
x.substitute(z,y)

x.set(y)
x.commit()

y = x.get(alias="BEAM",src=0x14000a00)
y[0].get()
