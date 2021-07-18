CFLAGS=-g -Wall
CC=arm-linux-gnueabihf-gcc
AR=ar
RANLIB=ranlib
LIBS=-L./ -lcgic

all: libcgic.a cgictest.cgi linux-cgiv.cgi capture

install: libcgic.a
	cp libcgic.a /usr/local/lib
	cp cgic.h /usr/local/include
	@echo libcgic.a is in /usr/local/lib. cgic.h is in /usr/local/include.

libcgic.a: cgic.o cgic.h
	rm -f libcgic.a
	$(AR) rc libcgic.a cgic.o
	$(RANLIB) libcgic.a

#mingw32 and cygwin users: replace .cgi with .exe

cgictest.cgi: cgictest.o libcgic.a
	arm-linux-gnueabihf-gcc cgictest.o -o cgictest.cgi ${LIBS}
	
linux-cgiv.cgi: cJSON.c linux-cgiv.o libcgic.a
	arm-linux-gnueabihf-gcc cJSON.c linux-cgiv.o -o linux-cgiv.cgi ${LIBS}
	
capture: capture.o libcgic.a
	arm-linux-gnueabihf-gcc capture.o -o capture ${LIBS}

clean:
	rm -f *.o *.a cgictest.cgi linux-cgiv.cgi capture

