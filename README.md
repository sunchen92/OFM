# OFM

This is the code repository for the OFM project.

The code is under development.

***** Github Usage *****

1. Only push codes when you are sure that they can safely run

2. Use 'git checkout -b xxx' to create your own branch and make changes to that. DO NOT modify master branch

3. How to push local commits without having to input username and password every time:
http://blog.csdn.net/xsckernel/article/details/8563993

4. Remeber to always use 'git pull origin master' to get the newest code on that branch before merging with master branch

***** Project Compilation *****

1. We can simple execute './compile.sh' to compile floodlight -- controller -- app in sequence

2. Manually install json-c:

```
apt-get install  git make gcc libtool autoconf automake
 
git clone https://github.com/json-c/json-c.git

cd json-c

sh autogen.sh

./configure

make

make install

mv json /usr/include
```



3. Compile the shared library for NFs:
```
cd shared/

make

make install
```
Update some of the configuration lines in the shared library for NFs configuration file at /usr/local/etc/sdmbn.conf to the appropriate values:
ctrl_ip = "yourControllerIP"

4. Patch and compile PRADS:
```
apt-get install libpcre3-dev libpcap-dev python-docutils

git clone https://github.com/gamelinux/prads.git

cd prads

git checkout 930ff5

patch -p1 < ../prads.patch

make

make install
```
5. Patch and compile Bro:
```
apt-get update

sudo apt-get -y install cmake make gcc g++ flex bison libpcap-dev libssl-dev python-dev swig zlib1g-dev libmagic-dev

cd boost_1_54_0

cp -r boost /usr/local/include

./bootstrap.sh --with-libraries=serialization

./b2 install

cp -r /usr/local/include/boost /usr/include/

cp -r /usr/local/lib/*boost* /usr/lib/

cd bro-2.1

patch -p1 < ../bro.patch

sh sdmbn_build.sh

export PATH=/usr/local/bro/bin:$PATH
```
6. Compile iptables:
```
apt-get install libnetfilter-conntrack-dev

cd iptables-sdmbn

sh sdmbn_build.sh
```

***** Project Running *****

1. In server0, start the controller application:
```
java -jar apps/SDMBNapps.jar -cf apps/testTimedMoveAll.prop
```
2. In server1, start sending packets:
```
python3 send_tcp.py 200
```
3. In server2 and server3, start Prads/Bro/iptables:
```
Prads:  prads -i eth1

Bro: bro -i eth1

iptables: ./iptables-sdmbn -i eth1
```
