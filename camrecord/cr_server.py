#!/usr/bin/env python
from options import Options
import SocketServer
import os
import sys
import time

options = Options(['dir', 'port'], ['log'], [])

class MyTCPHandler(SocketServer.StreamRequestHandler):
    def handle(self):
        # We fork the recorder with stdin and stderr pointing to the socket.
        # stdout is either our standard out, or just /dev/null.
        if os.fork() == 0:
            os.dup2(self.rfile.fileno(), 0)
            if options.log == None:
                fd=os.open("/dev/null", os.O_WRONLY)
                os.dup2(fd, 1)
                os.close(fd)
            os.dup2(self.wfile.fileno(), 2)
            os.execv("/reg/neh/home1/mcbrowne/daq_release/build/pdsapp/bin/x86_64-linux-opt/camrecord",
                     ["camrecord", "-s"])

if __name__ == "__main__":
    try:
        options.parse()
    except Exception, msg:
        options.usage(str(msg))
        sys.exit()
    os.chdir(options.dir)
    HOST, PORT = "0.0.0.0", int(options.port)
    if options.log != None and options.log != "-":
        fd=os.open(options.log, os.O_WRONLY)
        os.dup2(fd, 1)
        os.close(fd)

    # Create the server.
    server = SocketServer.TCPServer((HOST, PORT), MyTCPHandler)
    server.log = options.log
    server.serve_forever()
