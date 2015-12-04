################################################################
# File: Proto.pm
# Authors: Mark Hayden, Chet Murthy, 11/00
# Copyright 2000 Mark Hayden.  All rights reserved.
# See license.txt for further information.
################################################################

package Ensemble::Proto;

use Ensemble::Handle;

use strict;

use vars qw/@ISA/;

@ISA = qw(Ensemble::Handle);

sub new {
  my $class = shift;
  my $self = {};
  bless $self, $class;

  my $stack = shift;
  $self->{'stack'} = $stack;
  $self->{'_handle'} = Ensemble::Internal::proto_id_of_string($stack);

  return $self;
}

1;
