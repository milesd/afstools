PROGRAMS=afs-auth kauthtest

# For HP-UX
# XLDFLAGS = -lBSD

# For AIX
# XLDFLAGS = -lbsd -ls

# For Solaris
XLDFLAGS = /lib/libsocket.a /usr/ucblib/libucb.a /lib/libnsl.a /lib/libintl.a /lib/libdl.so

# AFSDIR = /afs/transarc.com/product/afs/3.3/@sys
AFSDIR = /usr/afsws
AFSLIBDIR = $(AFSDIR)/lib
AFSLIBS = -L$(AFSLIBDIR) -L$(AFSLIBDIR)/afs  -lkauth -lprot -lubik \
		-lauth -lrxkad -lsys -ldes -lrx -llwp -lcom_err -laudit \
		$(AFSLIBDIR)/afs/util.a
INCLUDES = -I$(AFSDIR)/include

LDFLAGS = $(AFSLIBS) $(XLDFLAGS)
CFLAGS = $(INCLUDES) -g -D_NO_PROTO

all: $(PROGRAMS)

.c: ; $(CC) $(CFLAGS) $@.c $(LDFLAGS) -o $@
