#!/usr/bin/env perl

use strict;
use warnings;
use v5.16;
use File::Basename qw(dirname);

###############################################################################

my $bin        = "bin/smokerand";
my @generators = @ARGV;

if (!@generators) {
	die(usage());
}

# Make sure all the passed in values are legit generators before we start
foreach my $gen (@generators) {
	my $ok = is_valid_generator($gen);

	if (!$ok) {
		die("Generator '$gen' not found\n");
	}
}

################################################################################

# Get the numbers for each generator
my $res = {};
foreach my $gen (@generators) {
	my $num = get_results($bin, $gen);

	$res->{$gen} = { gbs => $num, };
}

# Sort by the speed of the generator
my @sort = sort{ $res->{$a}->{gbs} <=> $res->{$b}->{gbs} } keys %$res;

########################################

print "\n";

# Longest string
my $max_len = max_length(keys(%$res));

foreach my $gen (@sort) {
	my $num = $res->{$gen}->{gbs};

	printf("%${max_len}s: %s%0.2f GiB/s%s\n", $gen, color('230'), $num, color());
}

###############################################################################
# Functions
###############################################################################

sub is_valid_generator {
	my $str = shift();

	my $bin_dir = dirname($bin);
	my $gen_file = "$bin_dir/generators/$str.so";

	if (!-r $gen_file) {
		#k($gen_file);
		return 0;
	}

	return $gen_file;
}

sub max_length {
	my $max = 0;

	foreach my $item (@_) {
		my $len = length($item);
		if ($len > $max) {
			$max = $len;
		}
	}

	return $max;
}

sub usage {
	my $ret = "Usage: $0 [generator1] [generator2] [generator3] ...\n";

	return $ret;
}

sub get_results {
	my ($bin, $gen) = @_;

	my @glob = glob("bin/generators/*$gen*");
	my $gen_file = $glob[0];

	if (!$gen_file) {
		die("Unable to find generator $gen\n");
	}

	$gen = color('white', $gen);

	print "Gathering results for $gen...\n";
	my $cmd = "$bin speed $gen_file 2> /dev/null";
	my $out = `$cmd`;
	my $num = 0;

	# (GiB/sec): 8.57523
	if ($out =~ /GiB\/sec\): ([\d.]+)/) {
		$num = $1;
	}

	return $num;
}

# String format: '115', '165_bold', '10_on_140', 'reset', 'on_173', 'red', 'white_on_blue'
sub color {
	my ($str, $txt) = @_;

	# If we're NOT connected to a an interactive terminal don't do color
	if (-t STDOUT == 0) { return $txt // ""; }

	# No string sent in, so we just reset
	if (!length($str) || $str eq 'reset') { return "\e[0m"; }

	# Some predefined colors
	my %color_map = qw(red 160 blue 27 green 34 yellow 226 orange 214 purple 93 white 15 black 0);
	$str =~ s|([A-Za-z]+)|$color_map{$1} // $1|eg;

	# Get foreground/background and any commands
	my ($fc,$cmd) = $str =~ /^(\d{1,3})?_?(\w+)?$/g;
	my ($bc)      = $str =~ /on_(\d{1,3})$/g;

	if (defined($fc) && int($fc) > 255) { $fc = undef; } # above 255 is invalid

	# Some predefined commands
	my %cmd_map = qw(bold 1 italic 3 underline 4 blink 5 inverse 7);
	my $cmd_num = $cmd_map{$cmd // 0};

	my $ret = '';
	if ($cmd_num)      { $ret .= "\e[${cmd_num}m"; }
	if (defined($fc))  { $ret .= "\e[38;5;${fc}m"; }
	if (defined($bc))  { $ret .= "\e[48;5;${bc}m"; }
	if (defined($txt)) { $ret .= $txt . "\e[0m";   }

	return $ret;
}

sub file_get_contents {
	open(my $fh, "<", $_[0]) or return undef;
	binmode($fh, ":encoding(UTF-8)");

	my $array_mode = ($_[1]) || (!defined($_[1]) && wantarray);

	if ($array_mode) { # Line mode
		my @lines  = readline($fh);

		# Right trim all lines
		foreach my $line (@lines) { $line =~ s/[\r\n]+$//; }

		return @lines;
	} else { # String mode
		local $/       = undef; # Input rec separator (slurp)
		return my $ret = readline($fh);
	}
}

sub file_put_contents {
	my ($file, $data) = @_;

	open(my $fh, ">", $file) or return undef;
	binmode($fh, ":encoding(UTF-8)");
	print $fh $data;
	close($fh);

	return length($data);
}

# Creates methods k() and kd() to print, and print & die respectively
BEGIN {
    if (!defined(&trim)) {
        *trim = sub {
            my ($s) = (@_, $_); # Passed in var, or default to $_
            if (length($s) == 0) { return ""; }
            $s =~ s/^\s*//;
            $s =~ s/\s*$//;

            return $s;
        }
    }

	if (eval { require Dump::Krumo }) {
		*k  = sub { Dump::Krumo::kx(@_) };
		*kd = sub { Dump::Krumo::kxd(@_) };
	} else {
		require Data::Dumper;
		*k  = sub { print Data::Dumper::Dumper(\@_) };
		*kd = sub { print Data::Dumper::Dumper(\@_); die; };
	}
}

# vim: tabstop=4 shiftwidth=4 noexpandtab autoindent softtabstop=4

