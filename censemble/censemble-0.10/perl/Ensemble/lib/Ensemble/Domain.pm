################################################################
# File: Domain.pm
# Authors: Mark Hayden, Chet Murthy, 11/00
# Copyright 2000 Mark Hayden.  All rights reserved.
# See license.txt for further information.
################################################################

package Ensemble::Domain;

sub newNetsim {
  my $class = shift;
  my $self = {};
  bless $self, $class;

  my $alarm = shift;
  $self->{'_alarm'} = $alarm;
  $self->{'_handle'} = Ensemble::Internal::netsim_domain($alarm->{'_handle'});

  return $self;
}


sub newReal {
  my $class = shift;
  my $self = {};
  bless $self, $class;

  my $alarm = shift;
  $self->{'_alarm'} = $alarm;
  $self->{'_handle'} = Ensemble::Internal::udp_domain($alarm->{'_handle'});

  return $self;
}

1;
