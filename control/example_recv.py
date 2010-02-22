#!/reg/g/pcds/package/python-2.5.2/bin/python
#

import os
import struct
import socket

HOST = ''
PORT = 10149

if __name__ == "__main__":
    import sys

    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.setsockopt(socket.SOL_SOCKET,socket.SO_REUSEADDR,1)
    s.bind((HOST,PORT))
    s.listen(1)
    conn, addr = s.accept()
    print 'Connected by', addr

    nrecv = 0

    while True:
        data = struct.unpack('<6I',conn.recv(24))
        print 'Received', data
        nc = data[4]
        while nc > 0:
            cdata = struct.unpack('<32sId',conn.recv(44))
            print 'Controls', cdata
            nc = nc - 1
        nm = data[5]
        if nm > 0:
            mdata = struct.unpack('<32sIdd',conn.recv(52))
            print 'Monitors', mdata
            nm = nm - 1
        
        result=1
        conn.send(struct.pack('<i',result))

        if nrecv > 0:
            conn.send(struct.pack('<i',result))
    
    conn.close()
    s.close()
