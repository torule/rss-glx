#!/usr/bin/env perl
use strict;

if (`ps x | grep ' xscreensaver ' | grep -v grep`) {
	print "XScreenSaver seems to be running. Exit it before running this script.\n";
	exit;
}

my $screensavers = { };

$screensavers->{'cyclone'} 	= '  GL:                "Cyclone"  cyclone --root                              \\n\\';
$screensavers->{'euphoria'}	= '  GL:               "Euphoria"  euphoria --root                             \\n\\';
$screensavers->{'fieldlines'} 	= '  GL:             "Fieldlines"  fieldlines --root                           \\n\\';
$screensavers->{'flocks'} 	= '  GL:                 "Flocks"  flocks --root                               \\n\\';
$screensavers->{'flux'} 	= '  GL:                   "Flux"  flux --root                                 \\n\\';
$screensavers->{'helios'} 	= '  GL:                 "Helios"  helios --root                               \\n\\';
$screensavers->{'hyperspace'} 	= '  GL:             "Hyperspace"  hyperspace --root                           \\n\\';
$screensavers->{'lattice'} 	= '  GL:                "Lattice"  lattice --root                              \\n\\';
$screensavers->{'plasma'} 	= '  GL:                 "Plasma"  plasma --root                               \\n\\';
$screensavers->{'skyrocket'} 	= '  GL:              "Skyrocket"  skyrocket --root                            \\n\\';
$screensavers->{'solarwinds'} 	= '  GL:             "Solarwinds"  solarwinds --root                           \\n\\';
$screensavers->{'colorfire'} 	= '  GL:              "Colorfire"  colorfire --root                            \\n\\';
$screensavers->{'hufo_smoke'} 	= '  GL:           "Hufo\'s Smoke"  hufo_smoke --root                           \\n\\';
$screensavers->{'hufo_tunnel'} 	= '  GL:          "Hufo\'s Tunnel"  hufo_tunnel --root                          \\n\\';
$screensavers->{'sundancer2'} 	= '  GL:             "Sundancer2"  sundancer2 --root                           \\n\\';
$screensavers->{'biof'} 	= '  GL:                   "BioF"  biof --root                                 \\n\\';
$screensavers->{'busyspheres'} 	= '  GL:            "BusySpheres"  busyspheres --root                          \\n\\';
$screensavers->{'spirographx'} 	= '  GL:            "SpirographX"  spirographx --root                          \\n\\';
$screensavers->{'matrixview'} 	= '  GL:             "MatrixView"  matrixview --root                           \\n\\';
$screensavers->{'lorenz'}	= '  GL:                 "Lorenz"  lorenz --root                               \\n\\';
$screensavers->{'drempels'}	= '  GL:               "Drempels"  drempels --root                             \\n\\';
$screensavers->{'feedback'}	= '  GL:               "Feedback"  feedback --root                             \\n\\';
$screensavers->{'pixelcity'}	= '  GL:             "Pixel City"  pixelcity --root                            \\n\\';

open XSCREENSAVER, "$ENV{'HOME'}/.xscreensaver";
my @xscreensaver_config_file = <XSCREENSAVER>;
close XSCREENSAVER;

open XSCREENSAVER, ">$ENV{'HOME'}/.xscreensaver";

my $programs_section_flag = 0;
foreach my $line (@xscreensaver_config_file) {
	if ($line =~ /^programs:/) {
		$programs_section_flag = 1;
	} elsif ($programs_section_flag) {
		if ($line =~ /\\\s+/) {
			foreach my $screensaver (keys %{$screensavers}) {
				if ($line =~ /\s$screensaver\s/) {
					delete $screensavers->{$screensaver};
				}
			}
		} else {
			foreach my $screensaver (keys %{$screensavers}) {
				print XSCREENSAVER "$screensavers->{$screensaver}\n";
			}

			$programs_section_flag = 0;
		}
	}
	print XSCREENSAVER "$line";
}

close XSCREENSAVER;
