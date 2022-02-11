#
# Copyright (c) 1988 Regents of the University of California.
# All rights reserved.
#
# Redistribution and use in source and binary forms are permitted
# provided that the above copyright notice and this paragraph are
# duplicated in all such forms and that any documentation, advertising
# materials, and other materials related to such redistribution and
# use acknowledge that the software was developed by the University
# of California, Berkeley.  The name of the University may not be
# used to endorse or promote products derived from this software
# without specific prior written permission.  THIS SOFTWARE IS PROVIDED
# ``AS IS'' AND WITHOUT ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING,
# WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF MERCHANTIBILITY AND
# FITNESS FOR A PARTICULAR PURPOSE.
#
# @(#)Makefile	5.1 (Berkeley) 10/18/88
#

LIBDIR = /usr/local/lib/
# Need extralibs for old versions of AFS
#EXTRALIBS = ${LIBDIR}libscrypt.a
AFSLIBS =  ${LIBDIR}afs/libkauth.a ${LIBDIR}afs/libprot.a ${LIBDIR}librxkad.a \
		   ${LIBDIR}librx.a ${LIBDIR}liblwp.a ${LIBDIR}afs/libauth.a \
		   ${EXTRALIBS} \
		   ${LIBDIR}afs/libsys.a ${LIBDIR}librxkad.a ${LIBDIR}libubik.a ${LIBDIR}libdes.a \
		   ${LIBDIR}afs/libcmd.a ${LIBDIR}afs/libcom_err.a ${LIBDIR}afs/util.a ${SCRYPT}

SCRYPT=
#SCRYPT=${LIBDIR}libscrypt.a

CDEFINES=
#CDEFINES = -DBOGUS -DBSD_43

KERBEROS = -DKERBEROS

INCLUDES =   -I${DESTDIR}/include -I/usr/local/include
CFLAGS=	-g ${CDEFINES} ${KERBEROS} ${INCLUDES} -DUSERPERMITS -DTELNETDHACK -DVICEIICOMPAT -DSHELLHACK
LIBC=	/lib/libc.a
SRCS=	cklog.c 
OBJS= 	cklog.o  

LIBS = $(RESTLIB) ${AFSLIBS} /usr/local/lib/libkrb.a

all: cklog

cklog: ${OBJS}
	${CC} ${CDEFINES} -o $@ ${CFLAGS} ${OBJS} ${LIBS} 

clean:
	rm -f ${OBJS} core login *.o *.a

cleandir: clean
	rm -f ${MAN} tags .depend

install: ${MAN} all
	- mkdir ${DESTDIR}bin
	install -s cklog ${DESTDIR}bin/cklog

lint: ${SRCS}
	lint ${CFLAGS} ${SRCS}

tags: ${SRCS}
	ctags ${SRCS}

system: install
