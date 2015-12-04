################################################################
# File: LayerState.pm
# Authors: Mark Hayden, Chet Murthy, 11/00
# Copyright 2000 Mark Hayden.  All rights reserved.
# See license.txt for further information.
################################################################

package Ensemble::LayerState;

sub new {
  my $class = shift;
  my $self = {};
  bless $self, $class;

  my %args = @_;

  $self->{'_appl_intf'} = $args{'appl_intf'};
  $self->{'_domain'} = $args{'domain'};
  $self->{'_alarm'} = $args{'alarm'};
  $self->{'_handle'} = Ensemble::Internal::layer_state_create($self->{'_appl_intf'},
							      $self->{'_domain'}->{'_handle'},
							      $self->{'_alarm'}->{'_handle'});

  $self->{'_appl_intf'}->{'state'} = $self;

  return $self;
}

sub compose {
  my $self = shift;
  my $endpt = shift;
  my $vs = shift;

  Ensemble::Internal::layer_compose($self->{'_handle'},$endpt->{'_handle'},
									Ensemble::View->newCopy($vs)->acquire_handle);
}

1;
