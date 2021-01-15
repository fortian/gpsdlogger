CC=gcc
CFLAGS=-g -ggdb -Wall -O2 -Werror -x c
CXX=gcc
CXXFLAGS=$(CFLAGS)
LDFLAGS=-g -ggdb -lm -lgps
ARCH=$(shell uname -m)
ifeq ($(ARCH),x86_64)
    ARCH=amd64
endif

.PHONY: all clean tarball deb

all: gpsdlogger

gpsdlogger: gpsdc.o gpsPub.o logging.o
	$(CC) -o $@ $^ $(LDFLAGS)

gpsPub.o: gpsPub.cpp gpsPub.h
gpsdc.o: gpsdc.c logging.h gpsPub.h
logging.o: logging.[ch]

clean:
	rm -f gpsdlogger *.o debian/control/control debian/control/md5sums \
	    debian/control.tar.gz debian/data/usr/bin/gpsdlogger \
	    debian/data.tar.gz gpsdlogger_1.1.1-1_$(ARCH).deb

tarball: clean
	(cd ..; tar -cvf gpsdlogger.tar gpsdlogger)

deb: gpsdlogger_1.1.1-1_$(ARCH).deb

gpsdlogger_1.1.1-1_$(ARCH).deb: debian/debian-binary debian/control.tar.gz \
    debian/data.tar.gz
	$(MAKE) -C debian $@

debian/control.tar.gz: debian/control/control debian/control/md5sums
	$(MAKE) -C debian $(@F)

debian/control/control: debian/data/usr/bin/gpsdlogger \
    debian/control.in
	$(MAKE) -C debian control/control

debian/control/md5sums: debian/data/usr/bin/gpsdlogger
	$(MAKE) -C debian control/md5sums

debian/data.tar.gz: debian/data/usr/bin/gpsdlogger
	$(MAKE) -C debian $(@F)

debian/data/usr/bin/gpsdlogger: gpsdlogger
	cp -f $^ $@
	strip $@
	ssh -x -l root localhost chown -v root:root ${PWD}/$@
