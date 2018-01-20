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
    # bw 50 为50Mbps  TCIntf : 被 TC（linux 下的 traffic control 的工具）自定义的接口，可以配置包括带宽、延迟、丢包率、最大队列长度等参数。
    intf = custom(TCIntf, bw=50)
    #CPULimitedHost 带cpu限制的host，cpu限了10%   sched 指定Host的CPU调度方式:rt或者cfs 绝对公平调度算法，CFS(completely fair schedule)
    host = custom(CPULimitedHost, sched='cfs', cpu=0.1)

    # Create data network
    # SingleSwitchTopo:单个交换机上挂载若干主机，主机序号按照从小到大的顺序依次挂载到交换机的各个端口上。
    # OVSSwitch:表示一台 openvswitch 交换机（需要系统中已经安装并配置好 openvswitch），基于ovs-vsctl 进行操作。目前所谓的 OVSKernelSwitch 实际上就是 OVSSwitch 
    # RemoteController:表示一个在 mininet 控制外的控制器，即用户自己额外运行了控制器，此处需要维护连接的相关信息。
    net = Mininet(topo=SingleSwitchTopo( k=5 ), switch=OVSSwitch,
            controller=RemoteController, intf=intf, host=host, 
            autoSetMacs=True, autoStaticArp=True)

    # Add management network
    # 建了一个s2的switch
    s2 = net.addSwitch('s2', cls=StandaloneSwitch)
    for i in range(1, len(net.hosts)+1):
        # host名为h1、h2...h6？
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
