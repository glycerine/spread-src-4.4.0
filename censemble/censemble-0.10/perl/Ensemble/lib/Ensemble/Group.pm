################################################################
# File: Group.pm
# Authors: Mark Hayden, Chet Murthy, 11/00
# Copyright 2000 Mark Hayden.  All rights reserved.
# See license.txt for further information.
################################################################

package Ensemble::Group;

use Ensemble::Handle;

use strict;

use vars qw/@ISA/;

@ISA = qw(Ensemble::Handle);

sub new {
  my $class = shift;
  my $self = {};
  bless $self, $class;

  my $na = shift;
  $self->{'name'} = $na;
  $self->{'_handle'} = Ensemble::Internal::group_named($na);

  return $self;
}

sub DESTROY {
  my $self = shift;
  if (exists $self->{'_handle'}) {
#	print STDERR (sprintf "[Ensemble::Group::DESTROY: 0x%08x]\n", $self->{'_handle'});
	Ensemble::Internal::group_id_free($self->{'_handle'});
	delete $self->{'_handle'};
  }
}

1;
