#!/usr/bin/perl

# release_if_necessary

# releases a volume if its RW copy has been updated more recently than
# any of its RO copies

# 7/14/00 Kevin Hildebrand (kevin@glue.umd.edu)
# $Id: release_if_necessary,v 1.4 2003/07/11 01:53:29 pkd Exp $

#------------------------------------------------------------------------------

require "ctime.pl";
require "timelocal.pl";

$| = 1;			# Flush after writing

$VOS = "/usr/sbin/vos";

%months = (
           'Jan',0,
           'Feb',1,
           'Mar',2,
           'Apr',3,
           'May',4,
           'Jun',5,
           'Jul',6,
           'Aug',7,
           'Sep',8,
           'Oct',9,
           'Nov',10,
           'Dec',11);


sub unstrdate {
    local ($strtime) = @_;
    local ($day, $monname, $date, $time, $year, $mon, $hr, $min, $sec);

    ($day, $monname, $date, $time, $year) = split (' ', $strtime);

    $mon = $months{$monname};
    ($hr, $min, $sec) = split (':', $time);

    &timelocal($sec, $min, $hr, $date, $mon, $year);
}

if ($#ARGV < 0) {
    print "usage: release_if_necessary volume-name\n";
    exit 1;
}

$vol = shift;
$args = join (' ', @ARGV);

# XXX - Don't concatenate $args here. Rather, use grep() or something
# to see if there's a -c argument and get the cell from there.
if ($args =~ /(-c(\S*)\s+\S+)/) {
    $cell = $1;
}

$vol =~ s/\.readonly$//;
$vol =~ s/\.backup$//;

open (VOS, "$VOS examine $vol $cell|") or
	die "***   Can't vos examine $vol $cell\n";
while (<VOS>) {
    if (/Last Update\s+(\S.*)$/) {
	$lastupdate = unstrdate($1);
    }
}
close (VOS);

if (!$lastupdate) {
    print STDERR "***   Can't find last update time for $vol.\n";
    exit 1;
}

open (VOS, "$VOS examine ${vol}.readonly $cell|")
	or die "***   Can't vos examine ${vol}.readonly $cell\n";
while (<VOS>) {
    if (/Creation\s+(\S.*)$/) {
	$ctime = unstrdate($1);
	if (!$creation || ($ctime < $creation)) {
	    $creation = $ctime;
	} 
    }
}
close (VOS);

if (!$creation) {
    print STDERR "***   Can't find last creation time for ${vol}.readonly.\n";
    exit 1;
}

if ($creation < $lastupdate) {
    my $err;

    print "Releasing $vol $cell...\n";
    # XXX - Use the list version of system() here, for better security.
    $err = system("$VOS release $vol $args");
    if ($err != 0)
    {
	print STDERR "***   Error releasing $vol $cell\n";
    }
} else {
    print "Volume $vol $cell is already up to date.\n";
}
