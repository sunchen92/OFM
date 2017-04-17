#!/bin/bash

if [[ $# -lt 2 || $# -eq 3 || $# -gt 5 ]]; then
    echo "Usage: $0 <send-interface> <trace-dir> [<listen-ip> <listen-port> [-k]]"
    exit 1
fi

INTERFACE=$1
TRACEDIR=$2

if [[ $# -gt 2 ]]; then
    LISTEN_IP=$3
    LISTEN_PORT=$4
else
    LISTEN_IP=`host $HOSTNAME | cut -f4 -d' '`
    LISTEN_PORT=8080
fi

if [[ $# -gt 4 ]]; then
    KEEP=$5
fi

echo "Send on interface $INTERFACE"
echo "Starting traceload server"
echo "Listening for commands on $LISTEN_IP:$LISTEN_PORT"
nc $KEEP -l $LISTEN_IP $LISTEN_PORT | while read COMMAND; do
    echo $COMMAND
    TOKENS=($COMMAND)
    ACTION=${TOKENS[0]}
    TRACE=${TOKENS[1]}
    RATE=${TOKENS[2]}
    NUMPKTS=${TOKENS[3]}
    if [[ $ACTION == "start" ]]; then
    	if [[ $NUMPKTS == -1 && $RATE == -1 ]]; then
        	tcpreplay -i $INTERFACE $TRACEDIR/$TRACE &
        elif [[ $NUMPKTS == -1 ]]; then
			tcpreplay -p $RATE -i $INTERFACE $TRACEDIR/$TRACE &
        elif [[ $NUMPKTS == -1 ]]; then
			tcpreplay -L $NUMPKTS -i $INTERFACE $TRACEDIR/$TRACE &
        else
			tcpreplay -L $NUMPKTS -p $RATE -i $INTERFACE $TRACEDIR/$TRACE &
		fi
    elif [[ $ACTION == "stop" ]]; then
        PSLINE=`ps uwax | grep tcpreplay | grep "$TRACE"`
        PSCOLS=($PSLINE)
        KILLPID=${PSCOLS[1]}
        kill -s SIGINT $KILLPID
     fi
done
