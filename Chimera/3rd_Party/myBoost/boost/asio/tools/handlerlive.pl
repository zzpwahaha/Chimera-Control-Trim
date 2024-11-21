#!/usr/bin/perl -w
#
# handlerlive.pl
# ~~~~~~~~~~~~~~
#
# A tool for post-processing the debug output generated by Asio-based programs
# to print a list of "live" handlers. These are handlers that are associated
# with operations that have not yet completed, or running handlers that have
# not yet finished their execution. Programs write this output to the standard
# error stream when compiled with the define `BOOST_ASIO_ENABLE_HANDLER_TRACKING'.
#
# Copyright (c) 2003-2021 Christopher M. Kohlhoff (chris at kohlhoff dot com)
#
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#

use strict;

my %pending_handlers = ();
my %running_handlers = ();

#-------------------------------------------------------------------------------
# Parse the debugging output and update the set of pending handlers.

sub parse_debug_output()
{
  while (my $line = <>)
  {
    chomp($line);

    if ($line =~ /\@asio\|([^|]*)\|([^|]*)\|(.*)$/)
    {
      my $action = $2;

      # Handler creation.
      if ($action =~ /^([0-9]+)\*([0-9]+)$/)
      {
        $pending_handlers{$2} = 1;
      }

      # Begin handler invocation. 
      elsif ($action =~ /^>([0-9]+)$/)
      {
        delete($pending_handlers{$1});
        $running_handlers{$1} = 1;
      }

      # End handler invocation.
      elsif ($action =~ /^<([0-9]+)$/)
      {
        delete($running_handlers{$1});
      }

      # Handler threw exception.
      elsif ($action =~ /^!([0-9]+)$/)
      {
        delete($running_handlers{$1});
      }

      # Handler was destroyed without being invoked.
      elsif ($action =~ /^~([0-9]+)$/)
      {
        delete($pending_handlers{$1});
      }
    }
  }
}

#-------------------------------------------------------------------------------
# Print a list of incompleted handers, on a single line delimited by spaces.

sub print_handlers($)
{
  my $handlers = shift;
  my $prefix = "";
  foreach my $handler (sort { $a <=> $b } keys %{$handlers})
  {
    print("$prefix$handler");
    $prefix = " ";
  }
  print("\n") if ($prefix ne "");
}

#-------------------------------------------------------------------------------

parse_debug_output();
print_handlers(\%running_handlers);
print_handlers(\%pending_handlers);
