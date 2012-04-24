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

  my $is_online = ($where ne "(null)");  
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
  
  print HTML '<html><body><table border="1">';
  print HTML '<tr>';
  print HTML '<th>nodename</th><th>known_first</th><th>known_last</th>';
  print HTML '<th>online_first</th><th>online_last</th><th>count_online</th><th>count_offine</th>';
  print HTML '<th>online_percent</th>';
  print HTML '</tr>';
  
  my $sth = $dbh->prepare("SELECT * FROM nodes ORDER BY nodename");
  $sth->execute() || die;
  while (my $n = $sth->fetchrow_hashref()) {
    my $count = $n->{count_online} + $n->{count_offline};
    my $percent = ($count > 0 ? round(($n->{count_online} / $count) * 100) . "%" : "0%");
    
    printf HTML "<tr><td>%s</td><td>%s</td><td>%s</td><td>%s</td><td>%s</td><td align=\"right\">%s</td><td align=\"right\">%s</td><td align=\"right\">%s</td>",
      $n->{nodename}, $n->{known_first}, $n->{known_last},
      $n->{online_first}, $n->{online_last},
      $n->{count_online}, $n->{count_offline},
      $percent;
  }
  $sth->finish();

  print HTML '</table></body></html>';
  close(HTML);
  rename("$html.$$", $html);
}
