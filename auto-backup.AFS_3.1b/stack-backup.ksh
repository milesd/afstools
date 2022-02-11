#!/bin/ksh

cd /usr/afs/backup
cell=`cat /usr/vice/etc/ThisCell`
case $cell in
        'foo.com' )
                day1=mon
                day2=tue
                day3=wed
                day4=thu
                day5=fri
                afsbackup="./afsbackup"
                butc="./butc"
                ;;
        'foo2.com' )
                day1=monday_week
                day2=tuesday_week
                day3=wednesday_week
                day4=thursday_week
                day5=friday_week
                afsbackup="afsbackup"
                butc="butc"
                ;;
        * )
                print "Unable to run in $cell"
                exit
                ;;
esac

week=""
while [[ "$week" = "" ]]
do
        read ans?"Enter the week of the cycle [1-4]: "
        [[ "$ans" -gt 0 && "$ans" -lt 5 ]] && week=$ans
done
day=""
while [[ "$day" = "" ]]
do
        read ans?"Enter the weekday [1-5]: "
        [[ "$ans" -gt 0 && "$ans" -lt 6 ]] && day=$ans
done
case $day in
        '5') dumplev="/${day1}${week}/${day2}${week}/${day3}${week}/${day4}${wee
k}/${day5}${week}" ;;
        '4') dumplev="/${day1}${week}/${day2}${week}/${day3}${week}/${day4}${wee
k}" ;;
        '3') dumplev="/${day1}${week}/${day2}${week}/${day3}${week}" ;;
        '2') dumplev="/${day1}${week}/${day2}${week}" ;;
        '1') dumplev="/${day1}${week}" ;;
esac

# loop until all dumps are requested
dumpstr=""
dobutc0="no"
dobutc1="no"
dobutc2="no"
read volset?"Enter the volume set to backup (Null to exit): "
while [[ "$volset" != "" ]]
do
        tc=""
        while [[ "$tc" = "" ]]
        do
                read ans?"Which butc is to dump $volset [0,1,2]? "
                [[ "$ans" -eq 0 || "$ans" -eq 1 || "$ans" -eq 2 ]] && tc=$ans
        done
        eval dobutc$tc="yes"
        dumpstr="${dumpstr}dump $volset $dumplev $tc\n"
        read volset?"Enter next volume set (Null to exit): "
done

# Get the display location
DISPLAY=""
while [[ $DISPLAY = "" ]]
do
        read DISPLAY?"Where should this be displayed (ex unix:0)? "
done
export DISPLAY
rc=1
while [[ $rc -ne 0 ]]
do
        print -n "Enter admin's "
        klog admin
        rc=$?
done
[[ $dobutc0 = "yes" ]] && aixterm -T 'butc 0' -e $butc 0 &
[[ $dobutc1 = "yes" ]] && aixterm -T 'butc 1' -e $butc 1 &
[[ $dobutc2 = "yes" ]] && aixterm -T 'butc 2' -e $butc 2 &
print -n "dumpstr=$dumpstr"
( print -n "$dumpstr" ; while read line ; do
print -- "$line"; done ) | $afsbackup
read ans?"Put the tar tape in /dev/rmt0 and press Enter:"
tar -cv .
unlog
