################################################################
# File: Addr.pm
# Authors: Mark Hayden, Chet Murthy, 11/00
# Copyright 2000 Mark Hayden.  All rights reserved.
# See license.txt for further information.
################################################################

package Ensemble::Addr;

use Ensemble::Handle;

use strict;

use vars qw/@ISA/;

@ISA = qw(Ensemble::Handle);

sub ADDR_UDP { return 0; }
sub ADDR_NETSIM { return 1; }

sub new {
  my $class = shift;
  my $self = {};
  bless $self, $class;

  my %args = @_;

  my $domain = $args{'domain'};
  my $ty = $args{'type'};
  $self->{'_domain'} = $domain;
  $self->{'_type'} = $ty;

  $self->{'_handle'} = Ensemble::Internal::domain_addr($domain->{'_handle'},$ty);

  return $self;
}

sub DESTROY {
  my $self = shift;
  if (exists $self->{'_handle'}) {
#	print STDERR (sprintf "[Ensemble::Addr::DESTROY: 0x%08x]\n", $self->{'_handle'});
	Ensemble::Internal::addr_id_free($self->{'_handle'});
	delete $self->{'_handle'};
  }
}
1;
