#!/bin/bash

export LDFLAGS="-lsdmbn -ljson-c -lboost_serialization"

./configure
sed -e 's/-lsdmbn -ljson-c -lboost_serialization//' -i build/src/CMakeFiles/bro.dir/link.txt
sed -e 's/-lcrypto/-lcrypto -lsdmbn -ljson-c -lboost_serialization/' -i build/src/CMakeFiles/bro.dir/link.txt
sed -e 's/-lsdmbn -ljson-c -lboost_serialization//' -i build/src/CMakeFiles/bro.dir/relink.txt
sed -e 's/-lcrypto/-lcrypto -lsdmbn -ljson-c -lboost_serialization/' -i build/src/CMakeFiles/bro.dir/relink.txt
make
make install
