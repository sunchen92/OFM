#Usage: python3 send_tcp.py 200
from pypacker import psocket
from pypacker.layer3 import ip
from pypacker.layer4 import tcp
import time
import random
import sys
import os

def send_pkt(pkt_num):
    ips = []
    for i in range(0, pkt_num):
        ips.append(str(random.randint(1, 254)) + '.' + str(random.randint(1, 254)) + '.' + str(random.randint(1, 254)) + '.' + str(random.randint(1, 254)))
    while True:
        for i in range(0, pkt_num):
            src_ip = ips[i]
            print('src ip : ' + src_ip)
            packet_ip = ip.IP(src_s=src_ip, dst_s="192.168.10.1") + tcp.TCP(dport=8888)
            psock = psocket.SocketHndl(mode=psocket.SocketHndl.MODE_LAYER_3, timeout=10)
            psock.send(packet_ip.bin(), dst=packet_ip.dst_s)
            time.sleep(0.001)
    psock.close()

if __name__ == '__main__':
    if len(sys.argv) != 2:
        print('Bad arguments')
    pkt_num = int(sys.argv[1])
    send_pkt(pkt_num)

