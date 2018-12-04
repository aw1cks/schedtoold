CC = gcc
CFLAGS = -O2 -Wall -g -Wduplicated-cond -Wduplicated-branches -Wlogical-op -Wrestrict -Wnull-dereference -Wjump-misses-init -Wdouble-promotion -Wshadow -Wformat=2
OBJ = utils.o pid-handler.o config-handler.o main.o
DEPS = utils.h pid-handler.h config-handler.h
INSTALL = install

all: schedtoold

schedtoold: $(OBJ) 
	$(CC) -o schedtoold $^ $ $(CFLAGS)


%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

install: schedtoold
	strip ./schedtoold
	$(INSTALL) -D -m 755 -s ./schedtoold /usr/bin/schedtoold
	$(INSTALL) -D  -m 644 ./init-scripts/systemd/schedtoold.service /usr/lib/systemd/system/schedtoold.service
clean:
	-rm -f ./$(OBJ) ./schedtoold

