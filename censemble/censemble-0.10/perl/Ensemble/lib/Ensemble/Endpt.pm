################################################################
# File: Endpt.pm
# Authors: Mark Hayden, Chet Murthy, 11/00
# Copyright 2000 Mark Hayden.  All rights reserved.
# See license.txt for further information.
################################################################

package Ensemble::Endpt;

sub new {
  my $class = shift;
  my $self = {};
  bless $self, $class;

  my $unique = shift;
  $self->{'_handle'} = Ensemble::Internal::endpt_id($unique->{'_handle'});

  return $self;
}

sub DESTROY {
  my $self = shift;
  if (exists $self->{'_handle'}) {
#	print STDERR (sprintf "[Ensemble::Endpt::DESTROY: 0x%08x]\n", $self->{'_handle'});
	Ensemble::Internal::endpt_id_free($self->{'_handle'});
	delete $self->{'_handle'};
  }
}

1;

