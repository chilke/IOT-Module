#!/usr/bin/env python

import struct
import time
import sys
import socket
import threading

LI = 'LI'
VN = 'VN'
MODE = 'MODE'
STRATUM = 'STRATUM'
POLL = 'POLL'
PRECISION = 'PRECISION'
ROOT_DELAY = 'ROOT_DELAY'
ROOT_DISPERSION = 'ROOT_DISPERSION'
REFERENCE_IDENTIFIER = 'REFERENCE_IDENTIFIER'
REFERENCE_TIMESTAMP_INT = 'REFERENCE_TIMESTAMP_INT'
ORIGINATE_TIMESTAMP_INT = 'ORIGINATE_TIMESTAMP_INT'
RECEIVE_TIMESTAMP_INT = 'RECEIVE_TIMESTAMP_INT'
TRANSMIT_TIMESTAMP_INT = 'TRANSMIT_TIMESTAMP_INT'
REFERENCE_TIMESTAMP_FRAC = 'REFERENCE_TIMESTAMP_FRAC'
ORIGINATE_TIMESTAMP_FRAC = 'ORIGINATE_TIMESTAMP_FRAC'
RECEIVE_TIMESTAMP_FRAC = 'RECEIVE_TIMESTAMP_FRAC'
TRANSMIT_TIMESTAMP_FRAC = 'TRANSMIT_TIMESTAMP_FRAC'

PACKET_FORMAT = '!B B b b 2i 9I'

Run = True
Offset = 0
Delay = 0

def dictToPacket(d):
    li_vn_mode = d.get(LI, 0) << 6
    li_vn_mode += d.get(VN, 3) << 3
    li_vn_mode += d.get(MODE, 3)
    return struct.pack(PACKET_FORMAT, li_vn_mode, d.get(STRATUM, 0), d.get(POLL, 0),
        d.get(PRECISION, 0), d.get(ROOT_DELAY, 0), d.get(ROOT_DISPERSION, 0),
        d.get(REFERENCE_IDENTIFIER, 0), d.get(REFERENCE_TIMESTAMP_INT, 0), d.get(REFERENCE_TIMESTAMP_FRAC, 0),
        d.get(ORIGINATE_TIMESTAMP_INT, 0), d.get(ORIGINATE_TIMESTAMP_FRAC, 0),
        d.get(RECEIVE_TIMESTAMP_INT, 0), d.get(RECEIVE_TIMESTAMP_FRAC, 0),
        d.get(TRANSMIT_TIMESTAMP_INT, 0), d.get(TRANSMIT_TIMESTAMP_FRAC, 0))

def packetToDict(p):
    tup = struct.unpack(PACKET_FORMAT, p[0:struct.calcsize(PACKET_FORMAT)])
    d = {}
    d[LI] = (tup[0] >> 6) & 3
    d[VN] = (tup[0] >> 3) & 7
    d[MODE] = (tup[0]) & 7
    d[STRATUM] = tup[1]
    d[POLL] = tup[2]
    d[PRECISION] = tup[3]
    d[ROOT_DELAY] = tup[4]
    d[ROOT_DISPERSION] = tup[5]
    d[REFERENCE_IDENTIFIER] = tup[6]
    d[REFERENCE_TIMESTAMP_INT] = tup[7]
    d[REFERENCE_TIMESTAMP_FRAC] = tup[8]
    d[ORIGINATE_TIMESTAMP_INT] = tup[9]
    d[ORIGINATE_TIMESTAMP_FRAC] = tup[10]
    d[RECEIVE_TIMESTAMP_INT] = tup[11]
    d[RECEIVE_TIMESTAMP_FRAC] = tup[12]
    d[TRANSMIT_TIMESTAMP_INT] = tup[13]
    d[TRANSMIT_TIMESTAMP_FRAC] = tup[14]

    return d

def reader():
    global Run, Offset
    while True:
        line = sys.stdin.readline().strip()
        print('Read: ' + line)
        if line == 'q':
            print('Quitting')
            break;
        try:
            toks = line.split(' ')
            if toks[0] == 'Offset':
                a = int(toks[1])
                Offset = a
                print('New Offset: ' + str(Offset))
            elif toks[0] == 'Delay':
                a = int(toks[1])
                Delay = a
                print('New Delay: ' + str(Delay))
        except Exception:
            continue
    Run = False

def dump(bytes):
    i = 0
    addrForm = '{0:04X}:'
    byteForm = ' {0:02X}'
    while i < len(bytes):
        s = addrForm.format(i)
        for x in range(0, 8):
            if i >= len(bytes):
                break
            s += byteForm.format(bytes[i])
            i += 1
        print(s)

if __name__ == '__main__':
    index = int(sys.argv[1])
    host = 'time.google.com'
    port = 123+index

    if index == 0:
        host = 'time.nist.gov'
    elif index == 1:
        host = 'pool.ntp.org'

    t = threading.Thread(target=reader)
    t.start()

    inSock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    inSock.bind(('0.0.0.0', port))
    inSock.settimeout(1)

    info = socket.getaddrinfo(host, 123, socket.AF_INET)[0]
    sockaddr = info[4]

    outSock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    outSock.settimeout(1)

    while Run:
        try:
            clientData, clientAddr = inSock.recvfrom(256)
            print('Received packet from client')
            dump(clientData)
            if len(clientData) == struct.calcsize(PACKET_FORMAT):
                print('Proper length')
                info = socket.getaddrinfo(host, 123, socket.AF_INET)[0]
                servAddr = info[4]
                outSock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
                outSock.settimeout(1)
                print('Sending packet to server')
                dump(clientData)
                outSock.sendto(clientData, servAddr)
                try:
                    servData, servAddr = outSock.recvfrom(256)
                    print('Received packet from server')
                    dump(servData)
                    servDict = packetToDict(servData)
                    servDict[RECEIVE_TIMESTAMP_INT] += Offset

                    clientData = dictToPacket(servDict)
                    if Delay > 0:
                        print('Delaying response: ' + str(Delay))
                        time.sleep(Delay)
                    print('Sending response to client')
                    dump(clientData)
                    inSock.sendto(clientData, clientAddr)
                except socket.timeout:
                    print('Out timeout')
            else:
                print('Received invalid length: ' + str(len(data)))
        except socket.timeout:
            continue

    t.join()
