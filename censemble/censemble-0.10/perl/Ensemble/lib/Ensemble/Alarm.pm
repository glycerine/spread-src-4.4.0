################################################################
# File: Alarm.pm
# Authors: Mark Hayden, Chet Murthy, 11/00
# Copyright 2000 Mark Hayden.  All rights reserved.
# See license.txt for further information.
################################################################

package Ensemble::Alarm;

sub newReal {
  my $class = shift;
  my $self = {};
  bless $self, $class;

  my %args = @_;
  $self->{'_sched'} = $args{'sched'};
  $self->{'_unique'} = $args{'unique'};
  $self->{'_handle'} = Ensemble::Internal::real_alarm($args{'sched'}->{'_handle'},$args{'unique'}->{'_handle'});

  return $self;
}

sub newNetsim {
  my $class = shift;
  my $self = {};
  bless $self, $class;

  my %args = @_;
  $self->{'_sched'} = $args{'sched'};
  $self->{'_unique'} = $args{'unique'};
  $self->{'_handle'} = Ensemble::Internal::netsim_alarm($args{'sched'}->{'_handle'},$args{'unique'}->{'_handle'});

  return $self;
}

1;
