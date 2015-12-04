################################################################
# File: Handle.pm
# Authors: Mark Hayden, Chet Murthy, 11/00
# Copyright 2000 Mark Hayden.  All rights reserved.
# See license.txt for further information.
################################################################

package Ensemble::Handle;

sub acquire_handle {
  my $self = shift;
  my $rv = $self->{'_handle'} ||
	die "Ensemble::Handle::acquire_handle: no handle in object <<$self>>";

  delete $self->{'_handle'};
  return $rv;
}

sub newWrapper {
  my $class = shift;
  my $self = {};
  bless $self, $class;

  my $handle = shift;

  $self->{'_handle'} = $handle;

#  print STDERR (sprintf "[%s->newWrapper(0x%08x)]\n", $class, $handle);

  return $self;
}

1;
