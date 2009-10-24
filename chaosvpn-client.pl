#!/usr/bin/perl

use constant VERSION => "0.12";

# 0.12 20081003 haegar@ccc.de
# - close generated tinc-up script with "exit 0", I do not want to give
#   a random exit code of the last command back to the tinc daemon
#
# 0.11 20060831 haegar@ccc.de
# - include script version as useragent in https request
# - do not include BindToAddress in tinc config if $my_ip is on the old
#   dummy default of 127.0.0.1, to avoid problems with legacy configfiles
#
# 0.10 20060813 haegar@ccc.de
# - included patch from packbart to use $my_ip for the BindToAddress tinc
#   configuration
#
# 0.09 20060222 haegar@ccc.de
# - do not set "IndirectData=yes" by default, only set it when the node has
#   "indirectdata=1" in the configfile
#
# 0.08 20060222 haegar@ccc.de
# - allow "route_network" and "route_network6" statements, which only add
#   a route to the kernel routing table, but not to the tinc-config of
#   a node - used when the node is a hub with own clients behind it
#
# 0.07 20060221 haegar@ccc.de
# - allow "_" in node-names, needed for mrmvpn
#
# 0.06 20060201 haegar@ccc.de
# - fix small error - the hidden= config setting did not work, I've checked
#   $peers instead of $peer, which naturally never matched
#
# 0.05 20040220 haegar@ccc.de
# - pfad zu /sbin/ip in eine Variable auslagern
#
# 0.04 20031203 haegar@ccc.de
# - config in ein externes configfile ausgelagert
# - abschiessen eines schon laufenden chaosvpn-daemons umgebaut,
#   da der normale weg nicht immer funktioniert
#
# 0.03 20031202 haegar@ccc.de
# - debug-logging per default an, damit man mehr sieht
# - unbenutzte config-variablen als solche kommentiert
#
# 0.02 20031020 haegar@ccc.de
# - peer-excludes funktionierten nicht
#
# v0.01 20031019 haegar@ccc.de
# - first revision

# JA, ICH WEISS HIER FEHLT NOCH VIEL UND ES IST AN DIVERSEN STELLEN
# EXTREM DRECKIG RUNTERGEHACKT ;)


use strict;
use LWP::UserAgent;
use HTTP::Request;
use CGI;
use Data::Dumper;


my $config		= "/etc/tinc/chaosvpn.conf";


# config-vars:
use vars qw(
	$my_peerid $my_vpn_ip $my_vpn_netmask $my_vpn_ip6
	@exclude
	$my_password $my_ip
	$networkname $tincd_bin $ifconfig $ifconfig6 $ip_bin
	$master_url $base $pidfile $tincd_debuglevel
	);

# old dead config vars, only kept for compatibility:
use vars qw(
	$my_external_ip
	);

# defaults:
$my_peerid		= "undef";
$my_vpn_ip		= "";
$my_vpn_netmask		= "255.255.255.255";
$my_vpn_ip6		= "";

$my_password		= ""; # unused
$my_ip			= ""; # default: do not bind to fixed source address

@exclude		= (); # links zu gewissen peer-ids nicht aufbauen

# ============================================================================
# you should'nt need to change anything below, 
# at least not for linux and chaosvpn

$networkname		= "chaos";
$tincd_bin		= "/usr/sbin/tincd";
$ip_bin			= "/sbin/ip";
$ifconfig		= "/sbin/ifconfig \$INTERFACE $my_vpn_ip netmask $my_vpn_netmask";
$ifconfig6		= "$ip_bin addr add $my_vpn_ip6/128 dev \$INTERFACE";

$master_url		= "https://www.vpn.hamburg.ccc.de/tinc-chaosvpn.txt";
$base			= "/etc/tinc/$networkname";
$pidfile		= "/var/run/tinc.$networkname.pid";
$tincd_debuglevel	= 3;


# config einlesen
require $config;

if (!-e "/dev/net/tun") {
	warn "/dev/net/tun missing - creating it";
	system("mkdir", "-p", "/dev/net") && die;
	system("mknod", "-m", "0600", "/dev/net/tun", "c", "10", "200") && die;
}

my $answer = call_out_to_server();
my $peers;
if ($answer) {
	#print $answer;
	$peers = parse_server_answer($answer);
	#print Dumper($peers);
} else {
	#die "we lost";
}

if ($peers) {
	# wir haben eine neue config bekommen

	eval {
		create_config($peers);
	};
	if ($@) {
		warn $@;
	}
}

# alten daemon beenden
if (-e $pidfile) {
	# get pid
	open(PIDFILE, "<$pidfile") || die "read error on pidfile $pidfile\n";
	my $pid = <PIDFILE>;
	chomp $pid;
	close(PIDFILE);

	if (($pid =~ /^\d+$/) && (kill(0, $pid))) {
		# prozess existiert, abschiessen
		kill "TERM", $pid;

		my $c;
		for ($c = 0; $c < 20; $c++) {
			# wir wollen nicht laenger als unbedingt
			# noetig warten, aber max 2sek

			select(undef, undef, undef, 0.1); # sleep 100ms
			last unless kill(0, $pid);
		}

		if ($c >= 20) {
			# existiert noch immer, do it the hard way
			# ist noetig wenn der tincd vorher probleme mit
			# seiner config hatte, dann reicht ein SIGTERM
			# nicht aus

			kill "KILL", $pid;
			select(undef, undef, undef, 0.1); # sleep 100ms
		}

		if (kill(0, $pid)) {
			# immer noch? da iss was fischig
			die "can't kill old tincd with pid $pid\n";
		}
	}
} else {
	# try it the old fashioned way, may not work
	system($tincd_bin, "-n", $networkname, "-k") || sleep 1;
}
# neuen daemon starten
system($tincd_bin, "-n", $networkname, "--debug", $tincd_debuglevel) && die;

exit(0);



sub call_out_to_server
{
	my $ua = new LWP::UserAgent;
	$ua->agent("ChaosVPNclient/" . VERSION);

	my $params = "id=" . CGI::escape($my_peerid) . 
		"&password=" . CGI::escape($my_password) .
		"&ip=" . CGI::escape($my_ip);

	#my $req = HTTP::Request->new(POST => $master_url);
	#$req->content_type("application/x-www-form-urlencoded");
	#$req->content($params);

	# testmode:
	my $req = HTTP::Request->new(GET => "$master_url?$params");

	my $res = $ua->request($req);

	if ($res->is_success) {
		my $answer = $res->content;
		return $answer;
	} else {
		#print Dumper($res);
		warn "Warning: " . $res->status_line() . "\n";
		return undef;
	}
}


sub parse_server_answer($)
{
	my ($answer) = @_;
	my $peers = {};

	my $current_peer = undef;
	my $peer = {};
	my $in_key = 0;
 
	foreach (split(/\n/, $answer)) {
		#print "debug: $_\n";

		s/\#.*$//;

		if (/^\s*\[(.*?)\]\s*$/) {
			if ($current_peer) {
				$peers->{$current_peer} = $peer;
			}
			$peer = {
				"use-tcp-only"	=> 0,
				"hidden"	=> 0,
				"silent"	=> 0,
				"port"		=> 655,
				};
			$current_peer = $1;
			$current_peer = undef unless ($current_peer =~ /^[a-z0-9_]+$/);
			$in_key = 0;
		} elsif ($current_peer) {
			if ($in_key) {
				$peer->{pubkey} .= $_;
				$peer->{pubkey} .= "\n";

				$in_key = 0
					if (/^-----END RSA PUBLIC KEY-----/);
			} elsif (/^\s*gatewayhost=(.*)$\s*/i) {
				$peer->{gatewayhost} = $1;
			} elsif (/^\s*owner=(.*)$\s*/i) {
				$peer->{owner} = $1;
			} elsif (/^\s*use-tcp-only=(.*)$\s*/i) {
				$peer->{"use-tcp-only"} = $1;
			} elsif (/^\s*network=(.*)\s*$/i) {
				push @{$peer->{networks}}, $1;
			} elsif (/^\s*network6=(.*)\s*$/i) {
				push @{$peer->{networks6}}, $1;
			} elsif (/^\s*route_network=(.*)\s*$/i) {
				push @{$peer->{route_networks}}, $1;
			} elsif (/^\s*route_network6=(.*)\s*$/i) {
				push @{$peer->{route_networks6}}, $1;
			} elsif (/^\s*hidden=(.*)\s*$/i) {
				$peer->{hidden} = $1;
			} elsif (/^\s*silent=(.*)\s*$/i) {
				$peer->{silent} = $1;
			} elsif (/^\s*port=(.*)\s*$/i) {
				$peer->{port} = $1;
			} elsif (/^\s*indirectdata=(.*)\s*$/i) {
				$peer->{indirect_data} = $1;
			} elsif (/^-----BEGIN RSA PUBLIC KEY-----/) {
				$in_key = 1;
				$peer->{pubkey} = $_ . "\n";
			}
		} elsif (/^\s*$/ || /^\s*\#/) {
			# ignore empty lines or comments
		} else {
			warn "unknown line: $_\n";
		}
	}

	# den letzten, noch offenen, peer auch in der struktur verankern
	if ($current_peer) {
		$peers->{$current_peer} = $peer;
	}

	return $peers;
}


sub create_config($)
{
	my ($peers) = @_;

	if (!-e "$base.first") {
		system("cp", "-r", "$base", "$base.first") && die;
	}

	if (-e "$base.new") {
		system("rm", "-r", "$base.new") && die;
	}
	if (-e "$base.old") {
		system("rm", "-r", "$base.old") && die;
	}


	system("mkdir", "-p", "$base.new") && die;
	system("mkdir", "-p", "$base.new/hosts") && die;
	system("cp", "$base/rsa_key.priv", "$base.new/rsa_key.priv") && die;
	chmod(0600, "$base.new/rsa_key.priv") || die;
	system("cp", "$base/rsa_key.pub", "$base.new/rsa_key.pub") && die;
	chmod(0600, "$base.new/rsa_key.pub") || die;

	# base config file erzeugen
	open(MAIN, ">$base.new/tinc.conf") || die "create tinc.conf failed";
	print MAIN "AddressFamily=ipv4\n";
	print MAIN "Device=/dev/net/tun\n";
	print MAIN "Interface=${networkname}_vpn\n";
	print MAIN "Mode=router\n";
	print MAIN "Name=$my_peerid\n";
	print MAIN "Hostnames=yes\n"; # unsure about this
	print MAIN "BindToAddress=${my_ip}\n" if ($my_ip && $my_ip ne "127.0.0.1");

	open(UP, ">$base.new/tinc-up") || die "create tinc-up failed";
	print UP "#!/bin/sh\n";
	print UP $ifconfig, "\n" if ($my_vpn_ip);
	print UP $ifconfig6, "\n" if ($my_vpn_ip6);

	PEERS: foreach my $id (keys %$peers) {
		my $peer = $peers->{$id};
		foreach (@exclude) {
			if ($id eq $_) {
				print "peer: $id -- excluded\n";
				next PEERS;
			}
		}

		print "peer: $id\n", Dumper($peer);

		open(PEER, ">$base.new/hosts/$id") || die "create hosts/$id failed";
		print PEER "Address=$peer->{gatewayhost}\n"
			if ($peer->{gatewayhost});
		print PEER "Cipher=blowfish\n";
		print PEER "Compression=0\n";
		print PEER "Digest=sha1\n";
		if ($peer->{indirect_data}) {
			print PEER "IndirectData=yes\n";
		} else {
			print PEER "IndirectData=no\n";
		}
		print PEER "Port=$peer->{port}\n";

		if ($my_vpn_ip) {
			foreach (@{$peer->{networks}}) {
				print PEER "Subnet=$_\n";
				print UP "$ip_bin -4 route add $_ dev \$INTERFACE\n"
					if ($id ne $my_peerid);
			}
			foreach (@{$peer->{route_networks}}) {
				print UP "$ip_bin -4 route add $_ dev \$INTERFACE\n"
					if ($id ne $my_peerid);
			}
		}
		if ($my_vpn_ip6) {
			foreach (@{$peer->{networks6}}) {
				print PEER "Subnet=$_\n";
				print UP "$ip_bin -6 route add $_ dev \$INTERFACE\n"
					if ($id ne $my_peerid);
			}
			foreach (@{$peer->{route_networks6}}) {
				print UP "$ip_bin -6 route add $_ dev \$INTERFACE\n"
					if ($id ne $my_peerid);
			}
		}

		print PEER "TCPonly=", ($peer->{"use-tcp-only"} ? "yes" : "no");
		print PEER "\n";

		print PEER $peer->{pubkey}, "\n";

		close(PEER) || die "write error hosts/$id";

		if ($id ne $my_peerid) {
			# den rest nur fuer die anderen hosts

			if ($peer->{gatewayhost} && !$peer->{hidden} && !$peers->{$my_peerid}->{silent}) {
				print MAIN "ConnectTo=$id\n";
			}

			if (-e "$base/hosts/$id-up") {
				system("cp", "$base/hosts/$id-up", "$base.new/hosts/$id-up") && die;
			}
			if (-e "$base/hosts/$id-down") {
				system("cp", "$base/hosts/$id-down", "$base.new/hosts/$id-down") && die;
			}
		}
	}

	close(MAIN) || die "write error tinc.conf";

	print UP "\nexit 0\n\n";
	close(UP) || die "write error tinc-up";
	system("chmod", "0700", "$base.new/tinc-up") && die;

	system("mv", "$base", "$base.old") && die;
	system("mv", "$base.new", "$base") && die;

	return 1;
}
