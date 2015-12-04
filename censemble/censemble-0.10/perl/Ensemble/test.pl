#!/home/$USER/Perl-5.6.0/bin/perl -w

# test.pl
# Authors: Mark Hayden, Chet Murthy, 11/00
# Copyright 2000 Mark Hayden.  All rights reserved.
# See license.txt for further information.

BEGIN { push(@INC,"./blib/lib","./blib/arch"); }

# Before `make install' is performed this script should be runnable with
# `make test'. After `make install' it should work as `perl test.pl'

######################### We start with some black magic to print on failure.

# Change 1..1 below to 1..last_test_to_print .
# (It may become useful if the test is moved to ./t subdirectory.)

BEGIN { $| = 1; print "1..1\n"; }
END {print "not ok 1\n" unless $loaded;}

use Ensemble;
$loaded = 1;
print "ok 1\n";

use Ensemble;
use Ensemble::Sched;
use Ensemble::Unique;
use Ensemble::Alarm;
use Ensemble::Group;
use Ensemble::Domain;
use Ensemble::Addr;
use Ensemble::Proto;
use Ensemble::ApplIntf;
use Ensemble::LayerState;
use Ensemble::Endpt;
use Ensemble::View;
use Data::Dumper;

use RandAppl;

#Ensemble::Internal::log_add("SYNCING");

$sched = Ensemble::Sched->new("RAND2");
$unique = Ensemble::Unique->new;
$alarm = Ensemble::Alarm->newNetsim(sched => $sched, unique => $unique);
$group = Ensemble::Group->new("RAND2");
$domain = Ensemble::Domain->newNetsim($alarm);
$addr = Ensemble::Addr->new(domain => $domain, type => Ensemble::Addr::ADDR_NETSIM());
$proto = Ensemble::Proto->new("BOTTOM:CHK_TRANS:MNAK:PT2PT:CHK_FIFO:CHK_SYNC:LOCAL:".
			      "TOP_APPL:STABLE:VSYNC:SYNC:".
			      "ELECT:INTRA:INTER:LEAVE:SUSPECT:".
			      "PRESENT:PRIMARY:XFER:HEAL:TOP");

@ls = ();
{
  for (my $i = 0 ; $i < $RandAppl::nmembers; $i++) {
	my $endpt = Ensemble::Endpt->new($unique);
	my $vs = Ensemble::View->newSingleton(proto => $proto, group => $group, endpt => $endpt, addr => $addr,
										  ltime => Ensemble::Internal::LTIME_FIRST(), etime => Ensemble::Internal::time_zero());
	
	$vs->quorum(int($RandAppl::nmembers/2)+1);
	my $appl_intf = RandAppl->new;
	$ls[$i] = Ensemble::LayerState->new(appl_intf => $appl_intf, domain => $domain, alarm => $alarm);
	print STDERR ("VIEW: ".$vs->to_string."\n");
	$ls[$i]->compose($endpt,$vs);
  }
}

Ensemble::Internal::appl_mainloop($alarm->{'_handle'});

######################### End of black magic.

# Insert your test code below (better if it prints "ok 13"
# (correspondingly "not ok 13") depending on the success of chunk 13
# of the test code):

