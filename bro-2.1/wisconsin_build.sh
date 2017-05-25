#!/bin/bash

BASEDIR="/scratch/${USER}/tools"
export CFLAGS="-I${BASEDIR}/json-c/include/ -I${BASEDIR}/sdmbn/include/ -g"
export CXXFLAGS="-I${BASEDIR}/json-c/include/ -I${BASEDIR}/sdmbn/include/ -g"
export LDFLAGS="-L${BASEDIR}/json-c/lib/ -L${BASEDIR}/sdmbn/lib/ -L/usr/local/lib/ -lsdmbn -ljson-c -lboost_serialization"

export LD_LIBRARY_PATH="${BASEDIR}/json-c/lib:${BASEDIR}/sdmbn/lib" 

./configure --prefix=${BASEDIR}/bro-sdmbn 
sed -e 's/-lsdmbn -ljson-c -lboost_serialization//' -i build/src/CMakeFiles/bro.dir/link.txt
sed -e 's/-lcrypto/-lcrypto -lsdmbn -ljson-c -lboost_serialization/' -i build/src/CMakeFiles/bro.dir/link.txt
make install
