#
#
#
BINDIR=bin/
ETCDIR=etc/
DESTDIR=/usr/local/
CC=/bin/cc
CFLAGS=-g $(PROF)
LIBDIR=/usr/local/lib/
PROGS=reauth
AFSLIBS =  ${LIBDIR}afs/libkauth.a ${LIBDIR}libubik.a ${LIBDIR}afs/libprot.a \
		${LIBDIR}librxkad.a\
		${LIBDIR}librx.a ${LIBDIR}liblwp.a ${LIBDIR}afs/libauth.a \
		${LIBDIR}afs/libsys.a ${LIBDIR}librxkad.a ${LIBDIR}libdes.a \
		${LIBDIR}afs/libcmd.a ${LIBDIR}afs/libcom_err.a \
		${LIBDIR}afs/util.a ${LIBDIR}afs/libaudit.a

CFLAGS= -DAFS

all: ${PROGS}

reauth: reauth.o ${AFSLIBS}
	${CC} -o reauth reauth.o ${AFSLIBS}

.c.o:
	${CC} ${CFLAGS} -O -c $<

install: reauth
	${INSTALL} -s reauth ${DESTDIR}${ETCDIR}



system: install

clean: 
	  rm -f *.o ${PROGS} *~




