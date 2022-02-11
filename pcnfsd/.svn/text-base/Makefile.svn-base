# RE_SID: @(%)/tmp_mnt/vol/dosnfs/shades_SCCS/unix/pcnfsd/v2/src/SCCS/s.Makefile 1.9 92/11/03 15:10:45 SMI
#
#	@(#)Makefile	1.9	11/3/92
#
# Makefile for rpc.pcnfsd
#

TARGET_OS = sunos

.INIT:
	-mkdir $(TARGET_OS)

LFLAGS_SVR4 = -L/usr/lib -Xt
LFLAGS_SUNOS = 

# uncomment BUGFLAG for debugging
#BUGFLAG = -g

COPTFLAGS = -O

CFLAGS_sunos = -DOSVER_SUNOS41
LIBS_SUNOS= -lrpcsvc

CFLAGS_svr4 = -DOSVER_SOLARIS2X 
LIBS_SVR4= -lrpcsvc -lsocket -lnsl

CFLAGS= $(BUGFLAG) $(COPTFLAGS) $(CFLAGS_$(TARGET_OS))

BIN= $(TARGET_OS)/rpc.pcnfsd
TST= $(TARGET_OS)/clnt.pcnfsd



all: $(BIN) $(TST)

SVR_OBJS=  \
	$(TARGET_OS)/pcnfsd_svc.o \
	$(TARGET_OS)/pcnfsd_xdr.o \
	$(TARGET_OS)/pcnfsd_v1.o \
	$(TARGET_OS)/pcnfsd_v2.o \
	$(TARGET_OS)/pcnfsd_misc.o \
	$(TARGET_OS)/pcnfsd_cache.o \
	$(TARGET_OS)/pcnfsd_print.o

TST_OBJS= \
	$(TARGET_OS)/pcnfsd_clnt.o \
	$(TARGET_OS)/pcnfsd_test.o \
	$(TARGET_OS)/pcnfsd_xdr.o

SVR_SRCS= $(SVR_OBJS:.o=.c)

TST_SRCS= $(TST_OBJS:.o=.c)

HDRS= pcnfsd.h common.h

.KEEP_STATE:

LINTFLAGS= -hbax

#
# Determine your OS type from this list, and uncomment the lines
# immediately following it if needed, If your OS type is not listed here,
# examine the various options described in "common.h" and create
# an entry in the PER O/S SECTION. Then add a corresponding
# section here.
#
# Solaris 2.x for SPARC
#CFLAGS = $(BUGFLAG) -DOSVER_SOLARIS2X 
#LIBS= -lrpcsvc -lsocket -lnsl
#
# SunOS 4.1[.x] for SPARC
#CFLAGS = $(BUGFLAG) -DOSVER_SUNOS41
#LIBS= -lrpcsvc
#
# SunOS 4.0.3[c] for SPARC, Sun386i or Sun 3
#CFLAGS = $(BUGFLAG) -DOSVER_SUNOS403C
#LIBS= -lrpcsvc
#
#
# rpcgen is somewhat braindamaged when it comes to including
# extensions to initialization and to the core loop. Therefore
# once a version of the generated code and headers are solid,
# it is necessary to comment out the targets for the generated
# files:
#	pcnfsd_xdr.c
#	pcnfsd_svc.c
#	pcnfsd_clnt.c
#	pcnfsd.h
#
# These are then placed under SCCS control and hacked.
# If you are porting to a new environment and things don't
# seem to work right, you may want to see what your local
# rpcgen will generate. To do this, save the original
# files, uncomment these targets, and make. (Or just run
# rpcgen by hand.) Compare things, especially the set of
# header files fulled in.
#
#pcnfsd_xdr.c: pcnfsd.h pcnfsd.x
#	rpcgen -c pcnfsd.x -o $@ 
#
#pcnfsd_svc.c: pcnfsd.h pcnfsd.x
#	rpcgen -s udp -s tcp -I pcnfsd.x -o $@
#
#pcnfsd.h: pcnfsd.x
#	rpcgen -h pcnfsd.x -o $@
#pcnfsd_clnt.c: pcnfsd.h pcnfsd.x
#	rpcgen -l pcnfsd.x -o $@


sunos/rpc.pcnfsd: $(SVR_OBJS) $(HDRS)
	$(CC)  -o $@ $(CFLAGS) $(LFLAGS_SUNOS) $(SVR_OBJS) $(LIBS_SUNOS)

svr4/rpc.pcnfsd: $(SVR_OBJS) $(HDRS)
	$(CC)  -o $@ $(CFLAGS) $(LFLAGS_SVR4) $(SVR_OBJS) $(LIBS_SVR4)


$(TARGET_OS)/pcnfsd_svc.o: $(HDRS) pcnfsd_svc.c
	$(CC)  -c $(CFLAGS) -o $@ pcnfsd_svc.c

$(TARGET_OS)/pcnfsd_xdr.o: $(HDRS) pcnfsd_xdr.c
	$(CC)  -c $(CFLAGS) -o $@ pcnfsd_xdr.c

$(TARGET_OS)/pcnfsd_v1.o: $(HDRS) pcnfsd_v1.c
	$(CC)  -c $(CFLAGS) -o $@ pcnfsd_v1.c

$(TARGET_OS)/pcnfsd_v2.o: $(HDRS) pcnfsd_v2.c
	$(CC)  -c $(CFLAGS) -o $@ pcnfsd_v2.c

$(TARGET_OS)/pcnfsd_misc.o: $(HDRS) pcnfsd_misc.c
	$(CC)  -c $(CFLAGS) -o $@ pcnfsd_misc.c

$(TARGET_OS)/pcnfsd_cache.o: $(HDRS) pcnfsd_cache.c
	$(CC)  -c $(CFLAGS) -o $@ pcnfsd_cache.c

$(TARGET_OS)/pcnfsd_print.o: $(HDRS) pcnfsd_print.c
	$(CC)  -c $(CFLAGS) -o $@ pcnfsd_print.c


sunos/clnt.pcnfsd: $(TST_OBJS) $(HDRS)
	$(CC)  -o $@ $(CFLAGS) $(LFLAGS_SUNOS) $(TST_OBJS) $(LIBS_SUNOS)

svr4/clnt.pcnfsd: $(TST_OBJS) $(HDRS)
	$(CC)  -o $@ $(CFLAGS) $(LFLAGS_SVR4) $(TST_OBJS) $(LIBS_SVR4)

$(TARGET_OS)/pcnfsd_clnt.o: $(HDRS) pcnfsd_clnt.c
	$(CC)  -c $(CFLAGS) -o $@ pcnfsd_clnt.c

$(TARGET_OS)/pcnfsd_test.o: $(HDRS) pcnfsd_test.c
	$(CC)  -c $(CFLAGS) -o $@ pcnfsd_test.c

# don't do this for now - devjani 10/1/92
#install: $(BIN)
#	install -d $(DESTDIR)/usr/etc
#	install -s $(BIN) $(DESTDIR)/usr/etc

tags: $(SRCS)
	ctags $(SVR_SRCS)

lint: $(SRCS)
	lint $(CFLAGS) $(LINTFLAGS) $(SVR_SRCS)
	lint $(CFLAGS) $(LINTFLAGS) $(TST_SRCS)

clean:
	rm -rf $(TARGET_OS)
