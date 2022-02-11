#
# Makefile for athena-specific programs.
#
#  $Id: Makefile,v 1.6 92/06/14 12:43:03 probe Exp $


DESTDIR=/mit/afsdev/bld/dest/
SRCDIR=/mit/afsdev/bld/dest/
INSTALL=${SRCDIR}bin/install

CFLAGS=-O -I/usr/athena/include -I${SRCDIR}include ${XCFLAGS}

INCLS=${SRCDIR}include/afs/auth.h

KLIBDIR=/usr/athena/lib
LIBS=	${SRCDIR}lib/afs/libprot.a ${SRCDIR}lib/afs/libcmd.a \
	${SRCDIR}lib/libubik.a ${SRCDIR}lib/afs/libauth.a \
	${SRCDIR}lib/afs/libcom_err.a ${SRCDIR}lib/afs/util.a \
	${SRCDIR}lib/librxkad.a \
	${SRCDIR}lib/afs/libsys.a ${SRCDIR}lib/librx.a \
	${SRCDIR}lib/liblwp.a \
	${KLIBDIR}/libkrb.a ${KLIBDIR}/libdes.a ${XLIBS}


PROGS=	asetkey newpag

FLAGS=	${MFLAGS} DESTDIR=${DESTDIR} SRCDIR=${SRCDIR} KLIBDIR=${KLIBDIR} \
	CC="${CC}" XCFLAGS="${XCFLAGS}" XLIBS="${XLIBS}"

all clean depend install::
	(cd aklog; make ${FLAGS} $@)

all:: $(PROGS)

asetkey.o:	asetkey.c ${INCLS}
newpag.o:	newpag.c
readcred.o:	readcred.c 

asetkey: asetkey.o $(LIBS)
	${CC} ${CFLAGS} -o asetkey asetkey.o $(LIBS)

aunlog: aunlog.o $(LIBS)
	${CC} ${CFLAGS} -o aunlog aunlog.o $(LIBS)

newpag: newpag.o $(LIBS)
	${CC} ${CFLAGS} -o newpag newpag.o $(LIBS)

readcred: readcred.o $(LIBS)
	${CC} ${CFLAGS} -o readcred readcred.o ${LIBS}

system: install
	(cd aklog; make ${FLAGS} system)

install:: $(PROGS)
	${INSTALL} asetkey ${DESTDIR}root.server/usr/afs/bin/
	${INSTALL} asetkey ${DESTDIR}etc/asetkey
	${INSTALL} newpag ${DESTDIR}bin/newpag

clean::
	rm -f *.o $(PROGS) 

depend::
