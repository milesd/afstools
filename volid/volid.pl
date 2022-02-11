#!/usr/bin/perl -w
# From Warren Yenson (wyenson@ms.com)
# Modified slightly by miles@cs.stanford.edu for use with my testing.

use strict;

our @forward = qw( + = 0 1 2 3 4 5 6 7 8 9 A B C D E F G H I J K L M N O P Q R S T
                   U V W X Y Z a b c d e f g h i j k l m n o p q r s t u v w x y z );

our %reverse;
for (my $i = 0; $i < scalar @forward; $i++) {
        $reverse{ $forward[$i] } = $i;
}

foreach my $arg (@ARGV) {
	my ($volid, $vnode, $inode, $uniq, $path);

	if ( $arg =~ /^\d+$/ ) {

		$volid = $arg;			 # $arg has to be the ID of a RW volume
		$path = "/vicep?/AFSIDat/" . num_to_char($volid & 0xff) . "/" . num_to_char($volid);



		printf "%9d:    %s\n", $volid, $path;

	} elsif ($arg =~ /^[\d.]+$/ ) {
		($volid, $vnode, $uniq) = split(/\./,$arg);			 # $arg has to be the ID of a RW volume

		#print "Inode is $inode\n";
		#printf("inode %d: %b\n",$inode,$inode); 
		#$vnode = $inode & 0x0000ffff;
		#print "Vnode is $vnode\n";

		#printf("vnode %d: %b\n",$vnode,$vnode); 

		#my $vlow = $vnode & 0xff;
		#printf("vlow: %b\n",$vlow); 
		#my $vhigh = $vnode & 0xff00;
		#$vhigh >>=8;
		#printf("vhigh: %b\n",$vhigh); 
		printf("%s\n", num_to_char($vnode));
		printf("%s\n", num_to_char($uniq & 0xff00));
		


		#$path = "/vicep?/AFSIDat/" . num_to_char($volid & 0xff) . "/" . num_to_char($volid) . '/' . num_to_char($inode << 16 & 0xff) . '/'
		#. num_to_char($inode << 24 & 0xff8) ;
		#$path = "/vicep?/AFSIDat/" . num_to_char($volid & 0xff) . "/" . num_to_char($volid) . '/' . num_to_char($vhigh) . '/'
		$path = "/vicep?/AFSIDat/" . num_to_char($volid & 0xff) . "/" . num_to_char($volid) . '/*/*/' . num_to_char($vnode) . '*';







		printf "%9d.%9d:    %s\n", $volid, $vnode, $path;

	} elsif ($arg =~ m|(/vicep.)?(/AFSIDat/)?([^/]+)/([^/]+)|) {

		my @dir_parts = split(/\//,$arg);
		my ($dir1, $dir2) = ($3, $4);
		$dir1 = $dir_parts[3];
		$dir2 = $dir_parts[4];
		my $v1 = $dir_parts[5];
		my $v2 = $dir_parts[6];

		my $path = "/" . $dir_parts[1] ."/AFSIDat/$dir1/$dir2/$v1/$v2/";
		my $volid = char_to_num($dir2);

		my $inode = ($volid << 16) | char_to_num($v1) | char_to_num($v2);

		printf "%9d:    %s (inode %s)\n", $volid, $path, $inode;
	}
}

sub char_to_num {
    my $dir = shift;
    my $result;

    my $shift = 0;
    foreach my $char (split //, $dir) {

	my $n = $reverse{ $char };

	$n <<= $shift;
	$shift += 6;

	$result |= $n;
    }

    return $result;
}

sub num_to_char {

    my $number = shift;
    my $string;

    unless ( $number ) {
        $string = $forward[0];
    } else {

        my @string;

        my $index = 0;
        while ($number) {
            $string[$index] = $forward[ $number & 0x3f ];
            $index++;
            $number >>= 6;
        }

        $string = join "", @string;
    }
    return $string;
}
