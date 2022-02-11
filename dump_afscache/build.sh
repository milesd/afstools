#!/bin/sh

AFSDIR=/afs/transarc.com/product/afs/3.3a/@sys
cc dump_afscache.c -c -DSHOW_NAMES -I$AFSDIR/include 
cc dump_afscache.o -o dump_afscache \
	-L$AFSDIR/lib -L$AFSDIR/lib/afs \
	-lvolser -lvldb -lubik -lauth -lrxkad -ldes -lrx -llwp -lsys -lkauth \
	-lcom_err -lcmd -laudit \
	$AFSDIR/lib/afs/util.a
