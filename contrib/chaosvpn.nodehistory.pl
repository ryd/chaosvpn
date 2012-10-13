#!/usr/bin/perl

use strict;
use DBI;
use DBD::SQLite;
use POSIX;
use Math::Round qw(:all);

my $tincctl = "/usr/sbin/tincctl";
my $network = "chaos";
my $dbfile = "/var/www/chaosvpn.nodes.sqlite";
my $html = "/var/www/chaosvpn.nodes.html";


my $dbh = DBI->connect("dbi:SQLite:dbname=$dbfile","","", {
  AutoCommit => 1,
  RaiseError => 1,
  sqlite_see_if_its_a_number => 1,
  sqlite_unicode => 1,
  });

my $now = POSIX::strftime("%Y-%m-%d %H:%M:%S", localtime());
my $unknown = "0000-01-01 00:00:00";

# Create Table
$dbh->do("
  CREATE TABLE IF NOT EXISTS nodes (
    nodename VARCHAR(255) NOT NULL DEFAULT '' PRIMARY KEY ASC,
    known_first DATETIME NOT NULL DEFAULT '$unknown',
    known_last DATETIME NOT NULL DEFAULT '$unknown',
    online_first DATETIME NOT NULL DEFAULT '$unknown',
    online_last DATETIME NOT NULL DEFAULT '$unknown',
    count_online BIGINT NOT NULL DEFAULT 0,
    count_offline BIGINT NOT NULL DEFAULT 0
    );
  ") || die;
$dbh->do("PRAGMA synchronous = OFF") || die;


# get all nodes
open(NODES, "$tincctl -n $network dump nodes |") || exit(1);
while (<NODES>) {
  next unless (/^(.*?) at (.*?) /);
  
  my $node = $1;
  my $where = $2;

  my $is_online = (($where ne "(null)") && ($where ne "unknown"));
  process_node($node, $is_online);
}
close(NODES);


output_html();


$dbh->disconnect();
exit(0);


sub process_node
{
  my ($node, $is_online) = @_;

  #printf "Process Node %s - Online: %s\n", $node, $is_online;
    
  my $record = $dbh->selectrow_hashref(
    "SELECT * FROM nodes WHERE nodename = ?",
    undef,
    $node);
  
  if (!$record) {
    $dbh->do("INSERT INTO nodes (nodename, known_first, online_first) VALUES (?, ?, ?)",
      undef,
      $node, $now, ($is_online ? $now : $unknown))
      || die;

    $record = $dbh->selectrow_hashref(
      "SELECT * FROM nodes WHERE nodename = ?",
      undef,
      $node) || die;
  }

  if ($is_online) {
    $dbh->do("UPDATE nodes SET known_last=?, online_last=?, count_online=count_online+1 WHERE nodename=?",
      undef,
      $now, $now, $node) || die;

    if ($record->{online_first} eq $unknown) {
      $dbh->do("UPDATE nodes SET online_first=? WHERE nodename=?",
        undef,
        $now, $node) || die;
    }
  } else {
    $dbh->do("UPDATE nodes SET known_last=?, count_offline=count_offline+1 WHERE nodename=?",
      undef,
      $now, $node) || die;
  }
}


sub output_html
{
  open(HTML, ">$html.$$") || die;
  
  print HTML '<html>', "\n";
  print HTML '<head>', "\n";
  print HTML '<title>ChaosVPN node list</title>', "\n";
  print HTML '  <style type="text/css">', "\n";
  print HTML '    body {';
  print HTML '      color: #000000;';
  print HTML '      background-color: #ffffff;';
  print HTML '    }';
  print HTML '    .bigtable {';
  print HTML '      border: 1px solid black;';
  print HTML '      empty-cells: show;';
  print HTML '    }';
  print HTML '    .bigtable th {';
  print HTML '      background-color: #cccccc;';
  print HTML '      padding: 2px;';
  print HTML '      margin: 2px;';
  print HTML '      font-weight: bold;';
  print HTML '    }';
  print HTML '    .bigtable td {';
  print HTML '      background-color: #eeeeee;';
  print HTML '      padding: 2px;';
  print HTML '      margin: 2px;';
  print HTML '    }';
  print HTML '    .deleted {';
  print HTML '      text-decoration: line-through;';
  print HTML '    }';
  print HTML '    .perfect {';
  print HTML '      color: #008800;';
  print HTML '      font-weight: bold;';
  print HTML '    }';
  print HTML '    .good {';
  print HTML '      color: #008800;';
  print HTML '    }';
  print HTML '    .neutral {';
  print HTML '      color: #000000;';
  print HTML '    }';
  print HTML '    .bad {';
  print HTML '      color: #888800;';
  print HTML '    }';
  print HTML '    .critical {';
  print HTML '      color: #880000;';
  print HTML '      font-style: italic;';
  print HTML '    }', "\n";
  print HTML '  </style>', "\n";
  print HTML '</head>', "\n";
  print HTML '<body>', "\n";
  print HTML '<table class="bigtable">', "\n";
  print HTML '<tr>';
  print HTML '<th>nodename</th><th>known_first</th><th>known_last</th>';
  print HTML '<th>online_first</th><th>online_last</th><th>count_online</th><th>count_offine</th>';
  print HTML '<th colspan="2">online_percent</th>';
  print HTML '</tr>', "\n";
  
  my $sth = $dbh->prepare("SELECT * FROM nodes ORDER BY lower(nodename)");
  $sth->execute() || die;
  while (my $n = $sth->fetchrow_hashref()) {
    my $count = $n->{count_online} + $n->{count_offline};
    my $percent = ($count > 0 ? round(($n->{count_online} / $count) * 100) : 0);

    my $css;
    if ($n->{known_last} ne $now) {
      $css = "deleted";
    } elsif ($count < 500) {
      $css = "neutral"; # new
    } elsif ($percent >= 99) {
      $css = "perfect";
    } elsif ($percent >= 90) {
      $css = "good";
    } elsif ($percent >= 20) {
      $css = "neutral";
    } elsif ($percent >= 2) {
      $css = "bad";
    } else {
      $css = "critical";
    }

    if ($n->{online_first} eq "0000-01-01 00:00:00") {
      $n->{online_first} = "never";
      $n->{online_last} = "&nbsp;";
    }

    my $onlinestate = ($n->{online_last} eq $now ? "&nbsp;Y" : "&nbsp;N");

    printf HTML "<tr><td class=\"$css\">%s</td><td class=\"$css\">%s</td><td class=\"$css\">%s</td><td class=\"$css\">%s</td><td class=\"$css\">%s</td><td class=\"$css\" align=\"right\">%s</td><td class=\"$css\" align=\"right\">%s</td><td class=\"$css\">%s</td><td class=\"$css\" align=\"right\">%s%%</td></tr>\n",
      $n->{nodename}, $n->{known_first}, $n->{known_last},
      $n->{online_first}, $n->{online_last},
      $n->{count_online}, $n->{count_offline},
      $onlinestate, $percent;
  }
  $sth->finish();

  print HTML '</table>', "\n";
  print HTML '</body></html>';
  close(HTML);
  rename("$html.$$", $html);
}
