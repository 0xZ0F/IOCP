#!/usr/bin/env python3

import socket
import argparse
import struct

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('ip', type=str, metavar="ip", help="IP address of server")
    parser.add_argument('port', type=int, metavar="port", help="Port of server")
    args = parser.parse_args()
        
    print("Tunnel client started.")
    
    try:
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as sock:
           sock.connect((args.ip, args.port))
           while True:
               data = input("> ")

               size = len(data)
               op = 0
               index = 0
               data = data.encode()

               buf = struct.pack(f'!LHH{len(data)}s', size, op, index, data)
               print(f"Send: {buf}")

               sock.send(buf)
               
               resp = sock.recv(1024)
               
               print(f"Recv: {resp}")

    except socket.error as err:
        print(err.strerror)

if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        print("")