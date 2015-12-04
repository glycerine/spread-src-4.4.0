################################################################
# File: ViewLocal.pm
# Authors: Mark Hayden, Chet Murthy, 11/00
# Copyright 2000 Mark Hayden.  All rights reserved.
# See license.txt for further information.
################################################################

package Ensemble::ViewLocal;

use Ensemble::Handle;

use strict;

use vars qw/@ISA/;

@ISA = qw(Ensemble::Handle);

sub rank {
  my $self = shift;
  return Ensemble::Internal::view_local_get_rank($self->{'_handle'});
}

sub addr {
  my $self = shift;
  return Ensemble::Addr->newWrapper(Ensemble::Internal::view_local_get_addr($self->{'_handle'}));
}

sub DESTROY {
  my $self = shift;
  if (exists $self->{'_handle'}) {
#	print STDERR (sprintf "[Ensemble::ViewLocal::DESTROY: 0x%08x]\n", $self->{'_handle'});
	Ensemble::Internal::view_local_free($self->{'_handle'});
	delete $self->{'_handle'};
  }
}

1;
