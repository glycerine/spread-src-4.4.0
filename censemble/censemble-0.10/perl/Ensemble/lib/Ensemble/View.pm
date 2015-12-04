################################################################
# File: View.pm
# Authors: Mark Hayden, Chet Murthy, 11/00
# Copyright 2000 Mark Hayden.  All rights reserved.
# See license.txt for further information.
################################################################

package Ensemble::View;

use Ensemble::Handle;

use strict;

use vars qw/@ISA/;

@ISA = qw(Ensemble::Handle);

sub newSingleton {
  my $class = shift;
  my $self = {};
  bless $self, $class;

  my %args = @_;

  $self->{'proto'} = $args{'proto'};
  $self->{'group'} = $args{'group'};
  $self->{'endpt'} = $args{'endpt'};
  $self->{'addr'} = $args{'addr'};

  $self->{'_handle'} = Ensemble::Internal::view_singleton($self->{'proto'}->{'_handle'},
														  $self->{'group'}->{'_handle'},
														  $self->{'endpt'}->{'_handle'},
														  $self->{'addr'}->{'_handle'},
														  $args{'ltime'},
														  $args{'etime'});

  return $self;
}

sub newCopy {
  my $class = shift;
  my $self = {};
  bless $self, $class;

  my $obj = shift;

  die "View::newCopy: argument must be a View"
	unless (UNIVERSAL::isa($obj,'Ensemble::View'));

  %{$self} = %{$obj};

  $self->{'_handle'} = Ensemble::Internal::view_state_copy($self->{'_handle'});

  return $self;
}

sub quorum {
  my $self = shift;

  if (@_) {
	my $n = shift;
	Ensemble::Internal::view_set_quorum($self->{'_handle'},int($n));
  } else {
	return Ensemble::Internal::view_get_quorum($self->{'_handle'});
  }
}

sub to_string {
  my $self = shift;
  return Ensemble::Internal::view_state_to_string($self->{'_handle'});
}

sub nmembers {
  my $self = shift;
  return Ensemble::Internal::view_get_nmembers($self->{'_handle'});
}

sub xfer_view {
  my $self = shift;
  return Ensemble::Internal::view_get_xfer_view($self->{'_handle'});
}

sub primary {
  my $self = shift;
  return Ensemble::Internal::view_get_primary($self->{'_handle'});
}

sub ltime {
  my $self = shift;
  return Ensemble::Internal::view_get_ltime($self->{'_handle'});
}

sub uptime {
  my $self = shift;
  return Ensemble::Internal::view_get_uptime($self->{'_handle'});
}

sub members_to_string {
  my $self = shift;
  return Ensemble::Internal::view_state_members_to_string($self->{'_handle'});
}

sub proto {
  my $self = shift;
  return Ensemble::Proto->newWrapper(Ensemble::Internal::view_get_proto($self->{'_handle'}));
}

sub group {
  my $self = shift;
  return Ensemble::Group->newWrapper(Ensemble::Internal::view_get_group($self->{'_handle'}));
}

sub DESTROY {
  my $self = shift;
  if (exists $self->{'_handle'}) {
#	print STDERR (sprintf "[Ensemble::View::DESTROY: 0x%08x]\n", $self->{'_handle'});
	Ensemble::Internal::view_state_free($self->{'_handle'});
	delete $self->{'_handle'};
  }
}

1;
