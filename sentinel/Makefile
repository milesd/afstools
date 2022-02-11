CFLAGS = -g -I/usr/local/include
PLIBS = -L/usr/local/lib -L/usr/local/lib/afs -lfsprobe -lafsint -lrx -llwp /usr/local/lib/afs/util.a
SLIBS = -L/usr/local/lib -lXm -lXt -lX11
OBJS = parser.o sentinel.o handler.o window.o callback.o
SRCS = parser.c sentinel.c handler.c window.c callback.c

all: sentinel-prober sentinel

sentinel: $(OBJS)
	$(CC) -o sentinel $(OBJS) $(SLIBS)

sentinel.o: sentinel.c sentinel.h

parser.o: parser.c sentinel.h

handler.o: handler.c sentinel.h

window.o: window.c sentinel.h

callback.o: callback.c sentinel.h

sentinel-prober: prober.o
	$(CC) -o sentinel-prober prober.o $(PLIBS)

install: sentinel sentinel-prober
	-mkdir $(DESTDIR)/etc
	install -c -s sentinel-prober $(DESTDIR)/etc
	-mkdir $(DESTDIR)/bin
	install -c -s sentinel $(DESTDIR)/bin

install-config:
	install -c -m 644 sentinel.config /afs/.andrew.cmu.edu/data/db/sentinel
