################################################################
# File: Unique.pm
# Authors: Mark Hayden, Chet Murthy, 11/00
# Copyright 2000 Mark Hayden.  All rights reserved.
# See license.txt for further information.
################################################################

package Ensemble::Unique;

sub new {
  my $class = shift;
  my $self = {};
  bless $self, $class;

  my $host = shift;
  my $pid = shift;
  $self->{'host'} = $host;
  $self->{'pid'} = $pid;

  $self->{'_handle'} = Ensemble::Internal::unique_create();

  return $self;
}

1;
