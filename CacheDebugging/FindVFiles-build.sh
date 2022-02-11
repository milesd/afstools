#!/bin/sh

# Insert the path to your AFS afsws environment for access to the
# supplied libraries and include files
#AFSDIR=/afs/transarc.com/product/afs/3.4a/@sys
cc FindVFiles.c -c -DSHOW_NAMES -I$AFSDIR/include 
cc FindVFiles.o -o FindVFiles \
	-L$AFSDIR/lib -L$AFSDIR/lib/afs \
	-lvolser -lvldb -lubik -lauth -lrxkad -ldes -lrx -llwp -lsys -lkauth \
	-lcom_err -lcmd -laudit \
	$AFSDIR/lib/afs/util.a 
