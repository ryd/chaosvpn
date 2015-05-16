#!/usr/bin/perl
#
# create graph off all known tincd nodes
#

# needs tinc v1.1-git to work, older versions do not support the used tincctl
# command.
#
# graphs using "fdp" from graphwiz.

use strict;

my $tincctl = "/usr/sbin/tinc";
my $network = "chaos";
my $png_output = "/var/www/chaosvpn.png";
my $svg_output = "/var/www/chaosvpn.svg";

my $grapher = "/usr/bin/fdp";
my $node_attr = '-Nshape=ellipse -Nfontname=Verdana -Nfontsize=10 -Nstyle=filled -Nfillcolor=#eeeeee -Ncolor=#000000';
my $edge_attr = '-Earrowhead=none -Efontsize=2 -Ecolor=#000044 -Estyle=solid';
my $graph_attr = '-Gcenter=1 -Gsplines=true -Goverlap=false';
my $png_attr = '-Tpng';
my $svg_attr = '-Tsvg';

my $include_offline = 1;


my %subnet_owners = ();
my %nodes = ();
my %colors = ();


# get all subnets, and collect node names with subnets
open(SUBNETS, "$tincctl -n $network dump subnets |") || exit(1);
while (<SUBNETS>) {
  chomp;
  next unless (/^(.*?) owner (.*?)$/);
  my $subnet = $1;
  my $node = $2;

  $subnet_owners{$node}++;
}
close(SUBNETS);

# get all nodes
open(NODES, "$tincctl -n $network dump nodes |") || exit(1);
while (<NODES>) {
  next unless (/^(.*?) at (.*?) /);
  
  my $node = $1;
  my $where = $2;
  $node =~ s/ id [0-9a-f]+$//;

  if (!defined($subnet_owners{$node}) || $subnet_owners{$node} == 0) {
    # skip nodes without subnets, they are deleted
    # if still existing they will appear in the edge output
    next;
  }

  $nodes{$node} = 0;
}
close(NODES);

# get all edges
my @edges;
my $maxlinks = 0;
open(EDGES, "$tincctl -n $network dump edges |") || exit(1);
while (<EDGES>) {
  next unless (/^(.*?) to (.*?) at/);

  my $from = $1;
  my $to = $2;
  $nodes{$from}++;
  if ($nodes{$from} > $maxlinks) {
    $maxlinks = $nodes{$from};
  }
  $nodes{$to}++;
  if ($nodes{$to} > $maxlinks) {
    $maxlinks = $nodes{$to};
  }

  push @edges, [$from, $to];
}
close(EDGES);


# process edges
my $links = "";
my $last = "";
my $color = "";
foreach (@edges) {
  my $from = $_->[0];
  my $to = $_->[1];

  my $weight;
  if ((($nodes{$from}/$maxlinks) > 0.75) && (($nodes{$to}/$maxlinks) > 0.75)) {
    $weight = 42;
  } else {
    $weight = 1;
  }

  if ($from ne $last) {
    my ($r, $g, $b) = (int(rand(16)), int(rand(16)), int(rand(16)));
    $color = sprintf("#%02x%02x%02x", $r*8, $g*8, $b*8);
    $colors{$from} = sprintf("#%02x%02x%02x", 192+$r*4, 192+$g*4, 192+$b*4);
    $last = $from;
  }

  $links .= " \"$from\" -> \"$to\" [weight=\"$weight\",arrowhead=\"normal\",arrowtail=\"none\",arrowsize=\"0.5\",color=\"$color\"];\n";
}


# process nodes
my $nodes = "";
my %levels = ();
foreach (sort(keys(%nodes))) {
  push @{$levels{$nodes{$_}}}, $_;
  if ($nodes{$_} > 0) {
    $nodes .= " \"$_\" [fillcolor=\"$colors{$_}\",label = \"$_\"];\n";
  } elsif ($include_offline) {
    $nodes .= " \"$_\" [label=\"$_\",shape=\"diamond\",fontsize=\"7\",color=\"#888888\",style=\"dashed\"];\n";
  }
}


my $subgraphs = "";
foreach my $l (keys(%levels)) {
  $subgraphs .= " {rank=same; ";
  
  foreach (@{$levels{$l}}) {
    $subgraphs .= "\"$_\" ";
  }

  $subgraphs .= "}\n";
}


my $digraph =
  "digraph {\n" .
  $nodes .
  $subgraphs .
  $links .
  "}\n";

unlink("$png_output.tmp");
open(GRAPHER, "| $grapher -o $png_output.tmp $graph_attr $edge_attr $node_attr $png_attr /dev/stdin") || exit 1;
print GRAPHER $digraph;
close(GRAPHER);
rename("$png_output.tmp", $png_output);

unlink("$svg_output.tmp");
open(GRAPHER, "| $grapher -o $svg_output.tmp $graph_attr $edge_attr $node_attr $svg_attr /dev/stdin") || exit 1;
print GRAPHER $digraph;
close(GRAPHER);
rename("$svg_output.tmp", $svg_output);

#print $digraph;

exit(0);
