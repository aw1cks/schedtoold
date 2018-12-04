CC = gcc
CFLAGS = -O2 -Wall -g
OBJ = utils.o pid-handler.o config-handler.o main.o
DEPS = utils.h pid-handler.h config-handler.h
INSTALL = install
DESTDIR = 


all: schedtoold

schedtoold: $(OBJ) 
	$(CC) -o schedtoold $^ $ $(CFLAGS)


%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

install: schedtoold
	strip ./schedtoold
	$(INSTALL) -D -m 755 -s ./schedtoold $(DESTDIR)/usr/bin/schedtoold
clean:
	-rm -f ./$(OBJ) ./schedtoold

