#!/bin/bash


echo "********** Start compiling floodlight **********\n"

cd floodlight/
ant clean
ant
cd ..

echo "********** Start compiling controller **********\n"

cd controller/lib
rm floodlight.jar
ln -s ../../floodlight/target/floodlight.jar
cd ..
ant clean
ant
cd ..

echo "********** Start compiling app **********\n"

cd apps
ant clean
ant
cd ..


echo ""
