################################################################
# File: EQueue.pm
# Authors: Mark Hayden, Chet Murthy, 11/00
# Copyright 2000 Mark Hayden.  All rights reserved.
# See license.txt for further information.
################################################################

package Ensemble::EQueue;

use Ensemble::Handle;

use strict;

use vars qw/@ISA/;

@ISA = qw(Ensemble::Handle);

sub add_xfer_done {
  my $self = shift;
  Ensemble::Internal::equeue_add_xfer_done($self->{'_handle'});
}

sub add_leave {
  my $self = shift;
  Ensemble::Internal::equeue_add_leave($self->{'_handle'});
}

sub add_cast {
  my $self = shift;
  my $s = shift;
  Ensemble::Internal::equeue_add_cast($self->{'_handle'},$s);
}

sub add_send1 {
  my $self = shift;
  my $dest = shift;
  my $s = shift;
  Ensemble::Internal::equeue_add_send1($self->{'_handle'},$dest,$s);
}

1;
