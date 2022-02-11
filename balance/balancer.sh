#!/bin/sh

#
# This script serves as an authenticating and
# error-reporting wrapper around the volume balancer.
#
# Invocation: balancer <config>
#
# like "balancer user"
#
# will run the balancer with user.cf in the logdir, and write
# out logs balance-user.<date>
#

PATH=/usr/local/bin:/usr/local/etc:/bin:/usr/bin:/usr/etc:/usr/sbin:/usr/stage/bin
export PATH

if [ $# != 1 ]; then
	echo "usage: balancer <config>"
	exit 1
fi

BB=acs+asg.log.balance
ADMIN=volume.admin
LOGDIR=/afs/andrew.cmu.edu/data/db/balance
VOLDIR=/afs/andrew.cmu.edu/data/db/afsadmin/volumes/latest/servers
export BB ADMIN LOGDIR VOLDIR

CONFIG=$1
export CONFIG

if [ ! -f ${LOGDIR}/${CONFIG}.cf ]; then
	mail -s "Config Failure" $BB << E_O_M
The balancer cannot find the config file ${LOGDIR}/${CONFIG}.cf,
so nothing is done.
E_O_M
	exit 1
fi

LOGFILE=${LOGDIR}/balance-${CONFIG}.`date +%y-%m-%d`
export LOGFILE

# Based on what machine we're on, get the password for volume.admin.
# This is ancient, historical, and quite unfortunate.
#
# This makes PASSWD a command, which when eval'd produces a password
if [ -f /.${ADMIN}_Password ]; then
        # admin.andrew.cmu.edu
        PASSWD="cat /.${ADMIN}_Password"
	export PASSWD
else
        # stage machines
        PASSWD="cat /usr/stage/tkt/voladmin_crypt | /usr/stage/bin/des -d -k `cat /usr/stage/tkt/keyfile`"
	export PASSWD
fi

#
# Run everything in a seperate PAG
#

pagsh << E_O_F

#
# Authenicate
#

PASSKEY=`eval \$PASSWD`

if [ "\${PASSKEY}x" = x ]; then
	mail -s "Password Failure" \$BB << E_O_M
The balancer cannot get the password for \$ADMIN so nothing was done.
E_O_M
	exit 1
fi

klog \$ADMIN \$PASSKEY > /dev/null 2>&1

if [ \$? != 0 ]; then
	mail -s "Password Failure" \$BB << E_O_M
The balancer cannot authenticate to \$ADMIN so nothing was done.
E_O_M
	exit 1
fi

PASSKEY=
if [ -z "\${VOLDIR}" ]; then
	balance -r -f \${LOGDIR}/\${CONFIG}.cf > \${LOGFILE} 2>&1
else
	balance -D \${VOLDIR} -r -f \${LOGDIR}/\${CONFIG}.cf > \${LOGFILE} 2>&1
fi
BEXIT=\$?

gzip -9 \${LOGFILE}
GEXIT=\$?

if [ \$GEXIT != 0 -o  \$BEXIT != 0 ]; then
	(
		if [ \$GEXIT != 0 ]; then
			echo Compression of \${LOGFILE} apparently failed. Please check.
			echo ""
		fi

		if [ \$BEXIT != 0 ]; then
			echo The balancer terminated with non-zero status. See \${LOGFILE} for the
			echo log of this run.
		fi
	) | mail -s "Balancer Terminated Abnormally" \$BB

	exit 1
fi

unlog
E_O_F

exit 0
