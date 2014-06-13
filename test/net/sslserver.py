# coding: utf-8
import os, sys
import socket
import ssl

# create cert.pem by "openssl req -new -x509 -days 365 -nodes -out cert.pem -keyout cert.pem"

def test():
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM, 0)
    sock = ssl.wrap_socket(sock,
                    server_side=True,
                    certfile="cert.pem",
                    keyfile=None,)
                    #ssl_version=ssl.PROTOCOL_TLSv1)

    sock.bind(('127.0.0.1', 9000))
    sock.listen(32)

    while True:
        newsock, addr = sock.accept()

        print 'read:', newsock.recv(1024)
        s = 'haha\r\n'
        print 'write:', s, newsock.send(s)

        newsock.close()

if __name__ == '__main__':
    test()


