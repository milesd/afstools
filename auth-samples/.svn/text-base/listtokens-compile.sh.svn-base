#! /bin/sh

AFSDIR=/usr/afsws
#AFSDIR=/afs/transarc.com/product/afs/3.3/@sys

cc -I$AFSDIR/include -I$AFSDIR/include/afs -g -c listtokens.c

cc -g -o listtokens listtokens.o \
   $AFSDIR/lib/afs/libauth.a  $AFSDIR/lib/librxkad.a $AFSDIR/lib/libdes.a  \
   $AFSDIR/lib/afs/libsys.a $AFSDIR/lib/librx.a $AFSDIR/lib/liblwp.a \
   $AFSDIR/lib/afs/libcmd.a $AFSDIR/lib/afs/util.a
