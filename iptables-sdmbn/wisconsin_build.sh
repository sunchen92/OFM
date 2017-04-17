#!/bin/bash

BASEDIR="/scratch/${USER}/tools"
export CFLAGS="-I${BASEDIR}/json-c/include/ -I${BASEDIR}/sdmbn/include/ -g"
export LDFLAGS="-L${BASEDIR}/json-c/lib/ -L${BASEDIR}/sdmbn/lib/"
export PREFIX="${BASEDIR}/iptables-sdmbn/"

make
make install
