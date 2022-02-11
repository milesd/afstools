# change this to wherever you have AFS installed
LOCAL = /usr/local
#VOS = "/usr/local/etc/vos"

AFSLIBS = -lvolser -lvldb -lkauth -lubik -lauth -lcmd -lrxkad -ldes -lrx -lsys -llwp -laudit\
	  -lcom_err ${LOCAL}/lib/afs/util.a
AFSLIBS+{pmax} = -lvolser -lvldb -lubik -lauth -lsys -lcmd -lrxkad -ldes -lrx -llwp\
	  -lcom_err -lkauth ${LOCAL}/lib/afs/util.a

LIBS = -L${LOCAL}/lib -L${LOCAL}/lib/afs ${AFSLIBS} ${YACCLIBS} -ll ${EXTRALIBS}
INCLUDES = -I${LOCAL}/include
DEFINES = $(AGENT_DEFINES) ${EXTRADEFINES} -DNEW_MATH=1
LDFLAGS=
LDFLAGS+{sun4_55}=-R /usr/local/lib
LDFLAGS+{hp7_90}=-Wl,+b,/usr/local/lib
LDFLAGS+{sgi_62}=-Wl,-rpath,/usr/local/lib

# Uncomment if you want flex
# LEX = flex

# Uncomment if you want bison
# YACC = bison -yd
# Comment if you use bison
YACCLIBS = -ly
YFLAGS = -d

# Optimization levels ...
OPT = -g
# OPT = -O

###
# Here begins architecture specific hackery
###

#
# Sun 4 (sun4c_413, sun4m_413, should work for any Sparc)
#
# The unbundled Sun C 1.1 acc compiler works
#
CC+{sun4} = acc
CC+{sun4_55} = gcc
EXTRALIBS+{sun4_55} = -lsocket -lnsl -lc /usr/ucblib/libucb.a
# CC = gcc

# Pmax (pmax_ul4)
#
# The Ultrix 4.2 cc cannot handle prototypes of function pointers
#
CC+{pmax} = gcc

#
# HP (hp700_ux90)
#
# Need _HPUX_SOURCE for Transarc headers
# Need -lBSD for Transarc libraries
#
# Notes for hp700_ux90 and gcc - an assembler warning is generated for balance.c
#	- program appears to work, though. gcc does *not* need _HPUX_SOURCE.
#
EXTRADEFINES+{hp700} = -D_HPUX_SOURCE
EXTRALIBS+{hp700} = -lBSD
# CC = c89
CC+{hp700} = cc -Aa
# CC = gcc

# 
# Linux (i386_linux2)
#
YACC+{i386_linux2}= bison -yd
YACCLIBS+{i386_linux2}=
YFLAGS+{i386_linux2}=
OPT+{i386_linux2}=-O4
#
# Default
#
# GCC is a reasonable ANSI compiler. Try it.
#
# CC = gcc
# CC = cc

#
# Turn on/off compilation of various agents.
#
# byweekuse - currently depends on a CMU-local source patch that
#		rolls the weekUse array in the volume header
#		into the volumeInfo structure returned by the
#		list volume calls. It has been submitted to Transarc.
#
# Do not use BYWEEKUSE unless you have this patch
#
AGENT_DEFINES = -DBYSIZE=1 -DBYNUMBER=1 -DBYWEEKUSE=1 -DBYOVERDRAFT=1

SRCS = balance.c afscall.c balutil.c xmalloc.c config-yacc.c config-lex.c\
       match.c hash.c mygetopt.c agent.c agent_bysize.c agent_bynumber.c \
       agent_byweekuse.c state.c agent_byoverdraft.c
OBJS = balance.o afscall.o balutil.o xmalloc.o config-yacc.o config-lex.o\
       match.o hash.o mygetopt.o agent.o agent_bysize.o agent_bynumber.o \
       agent_byweekuse.o state.o agent_byoverdraft.o

PROGRAM = balance

CFLAGS = ${OPT} ${INCLUDES} ${DEFINES}

all: ${PROGRAM}

${PROGRAM}: ${OBJS}
	$(CC) ${OBJS} -o $@ ${LDFLAGS} ${LIBS}

clean:
	rm -f *.o ${PROGRAM}

install: ${PROGRAM}
	-mkdir $(DESTDIR)/balance
	-mkdir $(DESTDIR)/balance/bin
	install -c -s ${PROGRAM} ${DESTDIR}/balance/bin
	install -c balancer.sh ${DESTDIR}/balance/bin
