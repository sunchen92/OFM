CC = gcc
LIBS = -levent -lz -lcrypto
CFLAGS = -Wall -std=gnu99 -g 
TARGETS = test 

.PHONY: all
.DEFAULT: all
all: $(TARGETS)

%.o: %.c
	$(CC) $(CFLAGS) $(LIBS) -c $^ -o $@

test: test.o compress.o
	$(CC) $(CFLAGS) $(LIBS) $^ -o $@

.PHONY: clean
clean:
	$(RM) $(TARGETS) *.o 
