#! /bin/sh
lsmod | grep pktgen || { echo -e " Should load pktgen.ko first! " 
    exit
}

# User Config Here -----------------------------------------------------------
MYDEV=em2 # use MYDEV to send pkts
PKT_STEP=10000 # 100pps STEP unit is nanosecond, 0 means the MAX speed
#PKT_STEP=1000000 # 1000pps
#PKT_STEP=100000 # 10000pps

PKT_LEN=64
DST_IP="10.0.0.11"
DST_MAC="f8:bc:12:4e:c0:8d"
echo -e "@@@@ flow pps= `[ $PKT_STEP -gt 0 ] && echo $((1000000000 / $PKT_STEP)) || echo line rate`"
echo -e "@@@@ pkt len = $PKT_LEN"

CLONE_SKB=1000000 # num of identical copies of the same pkt, 0 means alloc for every skb
SIRQ_AFTER_SEND_PKTNUM=10000 #do_softirq after send NUM pkts

# CAUTION PGDEV differ in diff config stage
pgset() 
{
    local result
    echo $1 > $PGDEV
    result=`cat $PGDEV | fgrep "Result: OK:"`
    if [ "$result" = "" ]; then
        cat $PGDEV | fgrep Result:
    fi
}
pg() 
{
    echo inject > $PGDEV
    cat $PGDEV
}
# Config Start Here -----------------------------------------------------------
# thread config
# Each CPU has own thread. Two CPU exammple. We add eth1, eth2 respectivly.
PGDEV=/proc/net/pktgen/kpktgend_0
echo "Configuring $PGDEV"
#echo "Removing all devices"
pgset "rem_device_all" 
echo "Adding $MYDEV to pktgen"
pgset "add_device $MYDEV" 
pgset "max_before_softirq $SIRQ_AFTER_SEND_PKTNUM"

# device config
# delay 0 means maximum speed.
PGDEV=/proc/net/pktgen/${MYDEV}
echo "Configuring $PGDEV"
# COUNT 0 means forever
pgset "count 0"
pgset "clone_skb $CLONE_SKB"
# NIC plus 4 bytes CRC
pgset "pkt_size $PKT_LEN"
pgset "delay $PKT_STEP"
pgset "dst $DST_IP" 
pgset "dst_mac $DST_MAC"

# Time to run
PGDEV=/proc/net/pktgen/pgctrl
echo "Running... ctrl^C to stop"
pgset "start" 
echo "Done"

# refer to /proc/net/pktgen/${MYDEV} for sending statistic
