#!/usr/bin/python

#coding=gbk

# Echo server program
import socket

HOST = '192.168.10.134'                 # Symbolic name meaning all available interfaces
PORT = 8080               # Arbitrary non-privileged port

with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
    s.bind((HOST, PORT))
    s.listen(1)
    conn, addr = s.accept()
    with conn:
        print('Connected by', addr)
        while True:
            data = conn.recv(1024)
            if not data: break
            #conn.sendall(data)
            #print(len(data), data, '\n')
            print('len=', len(data), '\n')
            if len(data) == 22:
                conn.send(b'GATOR 20\n')
            if len(data) == 411:
                conn.send(b'\x04\x00\x00\x00\x00\x00')
            if len(data) == 66:
                conn.send(b'\x01\xcd\x03\x00\x00\x00')
                conn.send(data_973)
                
