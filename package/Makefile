#
#	Makefile for package program by Russell J. Yount
#
#	(C) Copyright 10/12/86 by Carnegie Mellon University
#

MKDIRHIER 	= /usr/local/bin/mkdirhier
MKDIRHIER+{i386_linux2} = mkdir -p
MKDIRHIER+{i386_rh60} = mkdir -p
MKDIRHIER+{sun4x_56} = mkdir -p
MKDIRHIER+{sun4x_57} = mkdir -p
INSTALL		= /usr/local/bin/install

## Install levels
INSTALL_EXE	= $(INSTALL) -m 755 
INSTALL_BIN	= $(INSTALL_EXE) -s
INSTALL_TXT	= $(INSTALL) -m 644

## Installation directories
DESTDIR		= /usr/local/
prefix		= $(DESTDIR)
exec_prefix	= $(DESTDIR)
ETCDIR		= $(exec_prefix)/etc
MANDIR		= $(exec_prefix)/man
MAN1DIR		= $(MANDIR)/man1
MAN8DIR		= $(MANDIR)/man8
ROOTETCDIR	= $(exec_prefix)/root.client/etc

#############################################################################

# define NO_HOLES to disable creation of holey files
# SunOS cannot boot a holey vmunix
# So far, only pmax has tested OK
HOLES=-DNO_HOLES
HOLES+{pmax}=

# define VICE for vice group mapped to wheel
# define ANDREW to use cpu_os style cpu names
# define USES_UTIME to use utime instead of utimes
# define HAVE_MD5 to enable hashes

BASEDEFS=-DVICE -DANDREW -DHAVE_MD5
DEFS=${BASEDEFS}
DEFS+{hp700_ux90}=${BASEDEFS} -DUSES_UTIME
DEFS+{sgi}=${BASEDEFS} -DUSES_SYSMKDEV
DEFS+{sun4_54}=${BASEDEFS} -DUSES_SYSMKDEV
DEFS+{sun4_55}=${BASEDEFS} -DUSES_SYSMKDEV -DUSES_DIRENT
DEFS+{sun4x_56}=${BASEDEFS} -DUSES_SYSMKDEV -DUSES_DIRENT
DEFS+{sun4x_57}=${BASEDEFS} -DUSES_SYSMKDEV -DUSES_DIRENT -DUSES_SYSMACROS
DEFS+{i386_linux2}=${BASEDEFS} -DUSES_SYSMACROS
DEFS+{i386_linux2}=${BASEDEFS} -DUSES_SYSMACROS -DUSES_DIRENT
OPT=-O
# statically link this binary on hp/ux
OPT+{hp700_ux90}=-Aa -D_HPUX_SOURCE +O3 -Wl,-a,archive
OPT+{sun4_54}=-g -xs
OPT+{sun4_55}=-g -xs
OPT+{sun4x_56}=-g -xs
OPT+{sun4x_57}=-g -xs
CFLAGS=${OPT} ${HOLES} ${DEFS} -I/usr/local/include -Dstat=my_stat -Dlstat=my_lstat -Dfstat=my_fstat
CFLAGS+{sun4_54} = ${OPT} ${HOLES} ${DEFS} -I/usr/local/include -Dstat=my_stat -Dlstat=my_lstat -Dfstat=my_fstat
CFLAGS+{sun4_55} = ${OPT} ${HOLES} ${DEFS} -I/usr/local/include -Dstat=my_stat -Dlstat=my_lstat -Dfstat=my_fstat
CFLAGS+{sun4x_56} = ${OPT} ${HOLES} ${DEFS} -I/usr/local/include -Dstat=my_stat -Dlstat=my_lstat -Dfstat=my_fstat
CFLAGS+{sun4x_57} = ${OPT} ${HOLES} ${DEFS} -I/usr/local/include -Dstat=my_stat -Dlstat=my_lstat -Dfstat=my_fstat
EXLIBS+{sun4_54}=-ldl
EXLIBS+{sun4_55}=-ldl
EXLIBS+{sun4x_56}=-ldl
EXLIBS+{sun4x_57}=-ldl
EXTRALIBS=-L/usr/local/lib -lmd5 ${EXLIBS}
EXTRALIBS+{sgi_62}=/usr/local/lib/libdes.a ${EXLIBS}
CC+{sun4_413}=gcc

PROGRAM=package
OBJECTS=main.o loadfile.o node.o loadentry.o misc.o strsav.o check.o update.o rm.o mv.o cp.o mklostfound.o ln.o sync.o stat.o

all: ${PROGRAM}

stat.o:
	${CC} ${OPT} ${HOLES} ${DEFS} -I/usr/local/include -c stat.c

system: install

install: ${PROGRAM} package.8
	$(MKDIRHIER) $(ETCDIR) $(MAN8DIR) $(ROOTETCDIR)
	$(INSTALL_EXE) package $(ROOTETCDIR)
	$(INSTALL_EXE) package $(ETCDIR)
	$(INSTALL_TXT) package.8 $(MAN8DIR)

#	install -c package.help ${DESTDIR}help/

lint: *.c
	lint $?

clean:
	-rm -f core a.out *.o *.out *.BAK *.CKP
	rm -f ${OBJECTS} ${PROGRAM}

${PROGRAM}: ${OBJECTS}
	${CC} ${CFLAGS} ${OBJECTS} -o $@ ${EXTRALIBS}

${OBJECTS}: *.h
