################################################################
# File: Sched.pm
# Authors: Mark Hayden, Chet Murthy, 11/00
# Copyright 2000 Mark Hayden.  All rights reserved.
# See license.txt for further information.
################################################################

package Ensemble::Sched;

sub new {
  my $class = shift;
  my $self = {};
  bless $self, $class;

  my $na = shift;
  $self->{'name'} = $na;
  $self->{'_handle'} = Ensemble::Internal::sched_create($na);

  return $self;
}

1;
