PROGRAMS=crosslog

#SYS = pmax_ul43
#SYS = `sys`
#AFSDIR = /afs/transarc.com/product/afs/3.3a/$(SYS)
AFSDIR = /usr/afsws
AFSLIBDIR = $(AFSDIR)/lib
KAFSLIBS = -L$(AFSLIBDIR) -L$(AFSLIBDIR)/afs  -lkauth.krb -lprot -lubik \
		-lauth.krb -lrxkad -lsys -ldes -lrx -llwp -lcom_err -laudit \
		$(AFSLIBDIR)/afs/util.a
AFSLIBS  = -L$(AFSLIBDIR) -L$(AFSLIBDIR)/afs  -lkauth -lprot -lubik \
		-lauth -lrxkad -lsys -ldes -lrx -llwp -lcom_err -laudit \
		$(AFSLIBDIR)/afs/util.a
INCLUDES = -I$(AFSDIR)/include -I$(AFSDIR)/include/afs

LDFLAGS = $(KAFSLIBS) $(XLDFLAGS)
CFLAGS = -g -D_NO_PROTO $(INCLUDES)

all: $(PROGRAMS)
	cp $(PROGRAMS) /tmp

clean:
	rm -f $(PROGRAMS) *.o

.c: ; $(CC) $(CFLAGS) $@.c $(LDFLAGS) -o $@
