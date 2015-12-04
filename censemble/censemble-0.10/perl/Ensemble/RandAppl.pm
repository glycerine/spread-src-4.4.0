################################################################
# File: RandAppl.pm
# Authors: Mark Hayden, Chet Murthy, 11/00
# Copyright 2000 Mark Hayden.  All rights reserved.
# See license.txt for further information.
################################################################

package RandAppl;

use Ensemble::ApplIntf;

use strict;

use vars qw/@ISA $thresh $nmembers/;

@ISA = qw(Ensemble::ApplIntf);

use Ensemble::View;
use Ensemble::ViewLocal;
use Ensemble::EQueue;

$thresh = 5 ;
$nmembers = 7;

sub new {
  my $class = shift;
  my $self = {};
  bless $self, $class;

  return $self;
}

sub install {
  my $self = shift;
#  print STDERR "INSTALL($self)\n";

  my ($vl_h, $vs_h, $xmit_h) = @_;

  $self->{'vs'}->DESTROY if (exists $self->{'vs'});
  $self->{'ls'}->DESTROY if (exists $self->{'ls'});

  $self->{'vs'} = Ensemble::View->newWrapper($vs_h);
  my $vs = $self->{'vs'};

  $self->{'ls'} = Ensemble::ViewLocal->newWrapper($vl_h);
  my $ls = $self->{'ls'};

  $self->{'next'} = Ensemble::Internal::time_zero();

  $self->{'xmit'} = Ensemble::EQueue->newWrapper($xmit_h);

  delete $self->{'heard'} if (exists $self->{'heard'});

  for(my $i=0;$i<$vs->nmembers;$i++) {
	$self->{'heard'}->{$i} = 1;
  }
  delete $self->{'heard'}->{$ls->rank};

  print STDERR (sprintf "RAND:install:nmembers=%d rank=%d ltime=%qd xfer=%d prim=%d view=%s\n",
				$vs->nmembers, $ls->rank, $vs->ltime, $vs->xfer_view, $vs->primary,
				$self->{'vs'}->members_to_string);

  if ($vs->xfer_view) {
	$self->{'xmit'}->add_xfer_done;
  }
}

sub heartbeat {
  my $self = shift;
#  print STDERR "HEARTBEAT($self)\n";

  my $time = shift;

  if (!($time >= $self->{'next'})) { return; }

  $self->{'next'} = $time + Ensemble::Internal::sys_random(10000000);

  if (($self->{'vs'}->nmembers >= $thresh) &&
	  !(keys %{$self->{'heard'}}) &&
	  Ensemble::Internal::sys_random(20) == 0) {
	$self->{'xmit'}->add_leave;
  } elsif (Ensemble::Internal::sys_random(2) == 0) {
	$self->{'xmit'}->add_cast($self->make_msg);
  } else {
	my $dest = Ensemble::Internal::sys_random($self->{'vs'}->nmembers);
	if ($dest != $self->{'ls'}->rank) {
	  $self->{'xmit'}->add_send1($dest,$self->make_msg);
	}
  }
}

sub receive {
  my $self = shift;
#  print STDERR "RECEIVE($self)\n";
  my ($from, $is_send, $blocked, $msg) = @_;

  my ($md5,$buf) = unpack("a16 a*",$msg);
  my $new_md5 = Ensemble::Internal::md5($buf);

  delete $self->{'heard'}->{$from};

  if ($md5 ne $new_md5) {
	warn "ApplIntf::receive: md5 signatures do not match";
  } else {
#	print STDERR "RECEIVE($self) OK\n";
  }
}

sub make_msg {
  my $self = shift;

  my $buf = "";
  my $len = Ensemble::Internal::sys_random(1024);
  while($len-- > 0) {
	$buf .= pack("c",Ensemble::Internal::sys_random(256));
  }
  my $md5 = Ensemble::Internal::md5($buf);
  return ($md5.$buf);
}

sub exit {
  my $self = shift;
#  print STDERR "EXIT($self)\n";

  my $ltime = $self->{'vs'}->ltime;
  my $endpt = Ensemble::Endpt->new($self->{'state'}->{'_alarm'}->{'_unique'});
  die "no vs" unless (exists $self->{'vs'});
  die "no ls" unless (exists $self->{'ls'});

  my $vs =
	Ensemble::View->newSingleton(proto => $self->{'vs'}->proto,
								 group => $self->{'vs'}->group,
								 endpt => $endpt,
								 addr => $self->{'ls'}->addr,
								 ltime => $self->{'vs'}->ltime + 1,
								 etime => $self->{'vs'}->uptime);
  $vs->quorum(int($nmembers/2)+1);
  $self->{'state'}->compose($endpt,$vs);
}

1;
