INCLUDES=-I/usr/afsws/include
CFLAGS=-O -DSHOW_NAMES $(INCLUDES)
CC=/usr/local/bin/gcc
AFSBASE=/afs/transarc.com/product/afs/3.2/@sys
LIBPATH=-L$(AFSBASE)/lib -L$(AFSBASE)/lib/afs
LIBS= -lvolser -lacl -lvldb -lubik -lauth -lcmd -lrxkad -ldes -lrx -llwp -lcom_err -lkauth -lsys
AFSLIBDIR=$(AFSBASE)/lib/afs
OBJS=dump_afscache.o casestrcpy.o hostparse.o

dump_afscache: $(OBJS)
	$(CC) -o dump_afscache $(OBJS) $(LIBPATH) $(LIBS)

#vsutils.o: $(AFSLIBDIR)/libvolser.a(vsutils.o)
#       ar xv $(AFSLIBDIR)/libvolser.a vsutils.o

hostparse.o: $(AFSLIBDIR)/util.a(hostparse.o)
	ar xv $(AFSLIBDIR)/util.a hostparse.o

casestrcpy.o: $(AFSLIBDIR)/util.a(casestrcpy.o)
	ar xv $(AFSLIBDIR)/util.a casestrcpy.o

clean: 
	rm -f dump_afscache *.o

