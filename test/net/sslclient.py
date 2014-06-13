# coding: utf-8
import os, sys
import socket
import ssl

def test():
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM, 0)
    sock = ssl.wrap_socket(sock,
                    server_side=False,
                    certfile=None,
                    keyfile=None)
                    #ssl_version=ssl.PROTOCOL_TLSv1)

    sock.connect(('127.0.0.1', 9000))

    s = 'client\r\n'
    print 'write:', s, sock.send(s)
    print 'read:', sock.recv(1024)

    sock.close()

if __name__ == '__main__':
    test()


