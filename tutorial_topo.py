#!/usr/bin/python

from mininet.cli import CLI
from mininet.log import setLogLevel, info, error
from mininet.net import Mininet
from mininet.topo import SingleSwitchTopo
from mininet.node import RemoteController, OVSSwitch, Node, CPULimitedHost
from mininet.link import TCIntf, Intf
from mininet.util import custom

class StandaloneSwitch(OVSSwitch):
    def __init__(self, name, **params):
        OVSSwitch.__init__(self, name, failMode='standalone', **params)
    def start(self, controllers):
        return OVSSwitch.start(self, [ ])

if __name__ == '__main__':
    setLogLevel( 'info' )

    # Create rate limited interface
    intf = custom(TCIntf, bw=50)
    host = custom(CPULimitedHost, sched='cfs', cpu=0.1)

    # Create data network
    net = Mininet(topo=SingleSwitchTopo( k=5 ), switch=OVSSwitch,
            controller=RemoteController, intf=intf, host=host, 
            autoSetMacs=True, autoStaticArp=True)

    # Add management network
    s2 = net.addSwitch('s2', cls=StandaloneSwitch)
    for i in range(1, len(net.hosts)+1):
        host = net.get('h'+str(i))
        link = net.addLink(host, s2, intf=Intf)
        link.intf1.setIP('192.168.0.'+str(i), 24)
    gate = Node( 'gate', inNamespace=False )
    link = net.addLink(gate, s2)
    link.intf1.setIP('192.168.0.254', 24)

    # Set controller
#    c0 = RemoteController('c0', ip='127.0.0.1', port=6633)
#    net.controllers = [c0]
   
    # Run network
    net.start()

    CLI( net )
    net.stop()
