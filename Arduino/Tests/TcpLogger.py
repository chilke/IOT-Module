import socket

host = ''
port = 16384

with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as sock:
    while True:
        sock.bind((host, port))
        sock.listen(1)
        conn, addr = sock.accept()
        with conn:
            print('Connected by ', addr)
            s = ''
            while True:
                try:
                    c = conn.recv(1)
                    if not c:
                        break
                    if c.decode('utf-8')[0] == '\n':
                        print(s)
                        s = ''
                    else:
                        s += c.decode('utf-8')[0]
                except:
                    pass
            print(s)