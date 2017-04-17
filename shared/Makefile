SOURCES=SDMBNConn.c SDMBNCore.c SDMBNJson.c SDMBNConfig.c event.c discovery.c state.c
CC = gcc

ifeq (${PREFIX},)
PREFIX=/usr/local/
endif
LIBDIR=${PREFIX}/lib
INCLUDEDIR=${PREFIX}/include
CONFDIR=${PREFIX}/etc

CFLAGS += -Wall -g -DCONFIG_DIR='"${CONFDIR}/"'
LDFLAGS += -ljson-c -lpthread -lpcap -Wl,--no-undefined

build: libsdmbn.so

libsdmbn.so: ${SOURCES:.c=.o}
	$(CC) -o $@ -shared $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -fpic -c $^

install: build 
	install -d ${LIBDIR}
	install libsdmbn.so ${LIBDIR}/libsdmbn.so
	install -d ${INCLUDEDIR}
	install SDMBN.h ${INCLUDEDIR}/SDMBN.h
	install -d ${CONFDIR}
	if [ ! -f ${CONFDIR}/sdmbn.conf ]; then install sdmbn.conf ${CONFDIR}/sdmbn.conf; fi

clean:
	rm *.o *.so
