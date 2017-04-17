SOURCES=utils/network.c utils/build.c utils/parse.c main.c
CC = gcc
CFLAGS += -Wall -g
LDFLAGS += -lnetfilter_conntrack -ljson-c -lsdmbn -Wl,--no-undefined

ifeq (${PREFIX},)
PREFIX=/scratch/$(USER)/tools/iptables-sdmbn
endif
BINDIR=${PREFIX}/bin

build: iptables-sdmbn

iptables-sdmbn: ${SOURCES:.c=.o}
	$(CC) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $^ -o $@

install: build 
	install -d ${BINDIR}
	install iptables-sdmbn ${BINDIR}/iptables-sdmbn

clean:
	rm *.o
