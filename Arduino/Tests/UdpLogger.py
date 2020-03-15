import socket

with socket.socket(family=socket.AF_INET, type=socket.SOCK_DGRAM) as sock:
    sock.bind(('', 12345))
    while True:
        msg, (ip, port) = sock.recvfrom(1024)

        print("{}: {}".format(ip, msg.decode('utf-8')), end='')