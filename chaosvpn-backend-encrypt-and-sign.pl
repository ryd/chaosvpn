#!/usr/bin/perl

use constant VERSION => "0.4";

#
# this is the backend postprocessing for the chaosvpn client
#
#
# normal client users do not need this!
#
#
# to setup your own backend you need to install a bunch of additional
# debian packages, create your own rsa backend secret key, and change
# the paths below.
#
# create secret rsa key:
# openssl genrsa -out privkey.pem 4096
#
# export public key part:
# openssl rsa -in privkey.pem -pubout -out pubkey.pem
#


# v0.1 20100127 haegar@ccc.de
# - first revision, based on old chaosvpn perl client
# v0.2 20100412 haegar@ccc.de
# - disabled cleartext config file in webtree
# v0.3 20190120 haegar@ccc.de
# - added shortconfig.cfg creation for DN42 registry sync
# v0.3.1 20201004 haegar@ccc.de
# - server migration, set umask
# v0.4 20210405 haegar@ccc.de
# - recreate config text used by chaosvpn nodes, remove
#   all not needed things
# - added dn42-as field support and output into shortconfig.cfg


use strict;
use Data::Dumper;
use Archive::Ar;		# libarchive-ar-perl
use Compress::Zlib;		# libcompress-zlib-perl
use Crypt::OpenSSL::Random;     # libcrypt-openssl-random-perl
use Crypt::OpenSSL::RSA - RSA;  # libcrypt-openssl-rsa-perl
use Crypt::CBC;			# libcrypt-cbc-perl
use Crypt::Rijndael;		# libcrypt-rijndael-perl / AES

$| = 1;

my $GIT_DIR = $ENV{"GIT_DIR"};
if (!defined($GIT_DIR) || $GIT_DIR eq "" || !-d $GIT_DIR ) {
  print STDERR "\$GIT_DIR undefined!\n";
  exit(1);
}

my $destdir = `git config chaosvpn.destdir 2>/dev/null`;
$destdir =~ s/\s*$//s;
#my $cleartextconfig = `git config chaosvpn.cleartextconfig 2>/dev/null`;
#$cleartextconfig =~ s/\s*$//s;
my $signkey = "$GIT_DIR/chaosvpn-private/clearprivkey.pem";
my $signpubkey = "$GIT_DIR/chaosvpn-private/pubkey.pem";

umask(0022);

my $dn42_gateway_as = "64654";
my $dn42_default_min_length4 = "21";
my $dn42_default_max_length4 = "32";
my $dn42_default_min_length6 = "48";
my $dn42_default_max_length6 = "64";

# --- no changes needed below for simple usage ---


my $fileformat_version = "3";
# increase this number for every incompatible file format change


# check if git settings were there
if (!$destdir) {
  print STDERR "'git config chaosvpn.destdir' NOT DEFINED!\n";
  exit(1);
}
#if (!$cleartextconfig) {
#  print STDERR "'git config chaosvpn.cleartextconfig' NOT DEFINED!\n";
#  exit(1);
#}


my $config = read_file_into_string("<$GIT_DIR/export/chaosvpn-data.conf") || die "config read error\n";

my $sign_secret_key = read_file_into_string("<$signkey") || die "signkey read error\n";
my $sign_public_key = read_file_into_string("<$signpubkey") || die "signpubkey read error\n";

openssl_init();


# first: compatibility for old perl script:
#write_string_into_file(">$cleartextconfig", $config);
# second: compatibility for old sign method:
#my $signature = rsa_sign_data($config, $sign_secret_key);
#write_string_into_file(">$cleartextconfig.sig", $signature);


# third: new sign and encrypt
my $peers = parse_config($config);
if ($peers) {
	eval {
		# convert parsed config back to text,
		# to send to the nodes and leaving out
		# all gunk/comments
		$config = build_text_config($peers);
	};
	if ($@) {
		warn $@;
	}
	eval {
		create_peer_configs($peers, $config);
	};
	if ($@) {
		warn $@;
	}
	eval {
		create_shortconfig($peers);
	};
	if ($@) {
		warn $@;
	}
} else {
	die "read and parse failed!\n";
}

print "\nfinished.\n";
exit(0);

sub parse_config($)
{
	my ($answer) = @_;
	my $peers = {};

	my $current_peer = undef;
	my $peer = {};
	my $in_key = 0;
 
	foreach (split(/\n/, $answer)) {
		#print "debug: $_\n";

		s/^\s+//; # trim the start
		s/\#.*$//; # remove comments
		s/\s+$//; # trim the end

		if (/^\[(.*?)\]$/) {
			if ($current_peer) {
				$peers->{$current_peer} = $peer;
			}
			$peer = {
				"owner"			=> "",
				"gatewayhost"		=> "",
				"use-tcp-only"		=> 0,
				"hidden"		=> 0,
				"silent"		=> 0,
				"port"			=> 655,
				"networks"		=> [],
				"networks6"		=> [],
				"indirect_data"		=> 0,
				"pubkey"		=> "",
				"Ed25519PublicKey"	=> "",
				"pingtest"		=> "",
				"pingtest6"		=> "",
				"dn42-as"		=> [],
				"dn42-min-length4"	=> $dn42_default_min_length4,
				"dn42-max-length4"	=> $dn42_default_max_length4,
				"dn42-min-length6"	=> $dn42_default_min_length6,
				"dn42-max-length6"	=> $dn42_default_max_length6,
				};

			if ($dn42_gateway_as) {
				push @{$peer->{"dn42-as"}}, $dn42_gateway_as;
			}

			$current_peer = $1;
			$current_peer = undef unless ($current_peer =~ /^[a-z0-9_\-]+$/i);
			$in_key = 0;
		} elsif ($current_peer) {
			if ($in_key) {
				$peer->{pubkey} .= $_;
				$peer->{pubkey} .= "\n";

				$in_key = 0
					if (/^-----END RSA PUBLIC KEY-----/);
			} elsif (/^gatewayhost\s*=\s*(.*)$/i) {
				$peer->{gatewayhost} = $1;
			} elsif (/^owner\s*=\s*(.*)$/i) {
				$peer->{owner} = $1;
			} elsif (/^use-tcp-only\s*=\s*(.*)$/i) {
				$peer->{"use-tcp-only"} = $1;
			} elsif (/^network\s*=\s*(.*)$/i) {
				push @{$peer->{networks}}, $1;
			} elsif (/^network6\s*=\s*(.*)$/i) {
				push @{$peer->{networks6}}, $1;
			} elsif (/^hidden\s*=\s*(.*)$/i) {
				$peer->{hidden} = $1;
			} elsif (/^silent\s*=\s*(.*)$/i) {
				$peer->{silent} = $1;
			} elsif (/^port\s*=\s*(.*)$/i) {
				$peer->{port} = $1;
			} elsif (/^indirectdata\s*=\s*(.*)$/i) {
				$peer->{indirect_data} = $1;
			} elsif (/^Ed25519PublicKey\s*=\s*(.*)$/i) {
				$peer->{Ed25519PublicKey} = $1;
			} elsif (/^pingtest\s*=\s*(.*)$/i) {
				$peer->{"pingtest"} = $1;
			} elsif (/^pingtest6\s*=\s*(.*)$/i) {
				$peer->{"pingtest6"} = $1;
			} elsif (/^dn42-as\s*=\s*(.*)$/i) {
				push @{$peer->{"dn42-as"}}, $1;
			} elsif (/^dn42-min-length4\s*=\s*(.*)$/i) {
				$peer->{"dn42-min-length4"} = $1;
			} elsif (/^dn42-max-length4\s*=\s*(.*)$/i) {
				$peer->{"dn42-max-length4"} = $1;
			} elsif (/^dn42-min-length6\s*=\s*(.*)$/i) {
				$peer->{"dn42-min-length6"} = $1;
			} elsif (/^dn42-max-length6\s*=\s*(.*)$/i) {
				$peer->{"dn42-max-length6"} = $1;
			} elsif (/^-----BEGIN RSA PUBLIC KEY-----/) {
				$in_key = 1;
				$peer->{pubkey} = $_ . "\n";
			}
		} elsif ($_ eq "") {
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

sub build_text_config($)
{
	my ($peers) = @_;
	my $config =
		"#\n" .
		"# AUTOGENERATED FILE - DO NOT MODIFY MANUALLY!\n" .
		"# Edit only in chaosvpn-admin.git!\n" .
		"#\n" .
		"\n";

	PEERS: foreach my $id (sort(keys %$peers)) {
		my $peer = $peers->{$id};

		# do not include owner or dn42 attributes

		$config .= "\n[" . $id . "]\n";
		if ($peer->{hidden}) {
			$config .= "gatewayhost=localhost\n";
		} else {
			$config .= "gatewayhost=" . $peer->{gatewayhost} . "\n";
		}
		if ($peer->{port}) {
			$config .= "port=" . $peer->{port} . "\n";
		}
		$config .= "hidden=" . ($peer->{hidden} ? "1" : "0") . "\n";
		if ($peer->{silent}) {
			$config .= "silent=" . ($peer->{silent} ? "1" : "0") . "\n";
		}

		if ($peer->{"use-tcp-only"}) {
			$config .= "use-tcp-only=" . ($peer->{"use-tcp-only"} ? "1" : "0") . "\n";
		}
		if ($peer->{"indirect_data"}) {
			$config .= "indirectdata=" . ($peer->{"indirect_data"} ? "1" : "0") . "\n";
		}

		if (ref($peer->{networks}) eq "ARRAY") {
			foreach my $n (@{$peer->{networks}}) {
				$config .= "network=$n\n";
			}
		}
		if (ref($peer->{networks6}) eq "ARRAY") {
			foreach my $n (@{$peer->{networks6}}) {
				$config .= "network6=$n\n";
			}
		}

		# pingtest is commented in the configfile,
		# as the client does not understand it, but some
		# peers use the field for monitoring
		if ($peer->{"pingtest"} ne "") {
			$config .= "#pingtest=" . $peer->{"pingtest"} . "\n";
		}
		if ($peer->{"pingtest6"} ne "") {
			$config .= "#pingtest6=" . $peer->{"pingtest6"} . "\n";
		}

		if ($peer->{"Ed25519PublicKey"} ne "") {
			$config .= "Ed25519PublicKey=" . $peer->{"Ed25519PublicKey"} . "\n";
		}
		$config .= $peer->{pubkey};
	}

	return $config;
}

sub create_peer_configs($$)
{
	my ($peers, $config) = @_;

	if (-e "$destdir.new") {
		system("rm", "-r", "$destdir.new") && die;
	}
	if (-e "$destdir.old") {
		system("rm", "-r", "$destdir.old") && die;
	}

	system("mkdir", "-p", "$destdir.new") && die;

	chdir("$destdir.new") || die "chdir to $destdir.new failed: $!\n";

	open(INDEX, ">index.html") || die "create index.html failed\n";
	print INDEX "Nothing here to view.\n";
	close(INDEX);
	
	PEERS: foreach my $id (sort(keys %$peers)) {
		my $peer = $peers->{$id};

		my $ar = new Archive::Ar();

		print "\npeer: $id\n";
		#print Dumper($peer);

		$ar->add_data("chaosvpn-version", $fileformat_version);

		my $aeskey = Crypt::OpenSSL::Random::random_bytes(32);
		my $aesiv = Crypt::OpenSSL::Random::random_bytes(16);
		
		#write_string_into_file(">cleartext", $config) || die "write cleartext for $id failed: $!\n";
		#write_string_into_file(">pubkey.pem", $peer->{pubkey}) || die "write pubkey for $id failed: $!\n";

		#print "  add cleartext...";
		#$ar->add_data("cleartext", $config);
		#print ".\n";

		print "  compress config...";
		my $compressed_config = Compress::Zlib::compress($config, 9);
		print ".\n";
		
                print "  encrypt config...";
                my $encrypted_config = aes_encrypt($compressed_config, $aeskey, $aesiv);
                $ar->add_data("encrypted", $encrypted_config);
                print ".\n";
            
		print "  sign cleartext...";
		#system("/usr/bin/openssl", "dgst",
		#	"-sha512",
		#	"-sign", $signkey,
		#	"-out", "./signature",
		#	"./cleartext")
		#	&& die "digest for $id failed\n";
		my $signature = rsa_sign_data($config, $sign_secret_key);
		#write_string_into_file(">signature", $signature);
		$signature = aes_encrypt($signature, $aeskey, $aesiv);
		$ar->add_data("signature", $signature);
		print ".\n";

		print "  rsa part...";
		#my $rsa_cleartext =
		#  chr(length($aeskey)) . # length of aes key in bytes, 0-255
		#  chr(length($aesiv)) .	 # length of aes iv in bytes, 0-255
		#  $aeskey,
		#  $aesiv;
		my $rsa_cleartext = pack("CCA*A*",
		        length($aeskey), length($aesiv),
		        $aeskey,
		        $aesiv);
                my $rsa_enc = rsa_encrypt($rsa_cleartext, $peer->{pubkey});
                $ar->add_data("rsa", $rsa_enc);
		print ".\n";
		
		#system("/usr/bin/openssl", "dgst",
		#	"-sha512",
		#	"-verify", $signpubkey,
		#	"-signature", "./signature",
		#	"./cleartext")
		#	&& die "verify signature for $id failed\n";

		print "  create $id.dat...";
		#system("/usr/bin/ar", "-r",
		#	"./$id.dat",
		#	"./cleartext", "./signature")
		#	&& die "ar for $id failed\n";
		$ar->write("./$id.dat");
		print ".\n";
		
		#unlink("./cleartext");
		#unlink("./signature");
	}

	chdir("/") || die "chdir / failed: $!\n";

	if (-d "$destdir") {
		rename("$destdir", "$destdir.old") || die;
	}
	rename("$destdir.new", "$destdir") || die;

	return 1;
}

sub create_shortconfig($)
{
	my ($peers) = @_;
	my $shortconfig = "";

	PEERS: foreach my $id (sort(keys %$peers)) {
		my $peer = $peers->{$id};

		$shortconfig .= "\n[" . $id . "]\n";
		if (ref($peer->{networks}) eq "ARRAY") {
			foreach my $n (@{$peer->{networks}}) {
				$shortconfig .= "network=$n\n";
			}
		}
		if (ref($peer->{networks6}) eq "ARRAY") {
			foreach my $n (@{$peer->{networks6}}) {
				$shortconfig .= "network6=$n\n";
			}
		}

		if (ref($peer->{"dn42-as"}) eq "ARRAY") {
			foreach my $n (@{$peer->{"dn42-as"}}) {
				$shortconfig .= "dn42-as=$n\n";
			}
		}

		$shortconfig .= "dn42-min-length4=" . $peer->{"dn42-min-length4"} . "\n";
		$shortconfig .= "dn42-max-length4=" . $peer->{"dn42-max-length4"} . "\n";
		$shortconfig .= "dn42-min-length6=" . $peer->{"dn42-min-length6"} . "\n";
		$shortconfig .= "dn42-max-length6=" . $peer->{"dn42-max-length6"} . "\n";
	}

	write_string_into_file(">$destdir/shortconfig.cfg", $shortconfig);

	return 1;
}

sub openssl_init()
{
  if (open(URANDOM, "</dev/urandom")) {
    my $buffer = "";
    read URANDOM, $buffer, 512;
    close(URANDOM);
    if (defined($buffer) && ($buffer ne "")) {
      Crypt::OpenSSL::Random::random_seed($buffer);
    }
  }  
  Crypt::OpenSSL::RSA->import_random_seed();
}

sub rsa_sign_data($$)
{
  my ($data, $privkey) = @_;
  
  my $rsa_priv = Crypt::OpenSSL::RSA->new_private_key($privkey);
  $rsa_priv->use_sha512_hash();
  return $rsa_priv->sign($data);
}

sub rsa_verify_data($$$)
{
  my ($data, $signature, $pubkey) = @_;
  
  my $rsa_pub = Crypt::OpenSSL::RSA->new_public_key($pubkey);
  $rsa_pub->use_sha512_hash();
  return $rsa_pub->verify($data, $signature);
}

sub rsa_encrypt($$)
{
  my ($data, $pubkey) = @_;
  
  my $rsa_pub = Crypt::OpenSSL::RSA->new_public_key($pubkey);
  $rsa_pub->use_pkcs1_oaep_padding();
  return $rsa_pub->encrypt($data);
}

sub aes_encrypt($$$)
{
  my ($data, $aeskey, $aesiv) = @_;
  
  my $cipher = Crypt::CBC->new(
    -cipher		=> "Crypt::Rijndael",
    -padding		=> "standard",
    -header		=> "none",
    -literal_key	=> 1,
    -key		=> $aeskey,
    -iv			=> $aesiv,
    -blocksize		=> 16,
    );
  return $cipher->encrypt($data);
}

sub aes_decrypt($$$)
{
  my ($ciphertext, $aeskey, $aesiv) = @_;
  
  my $cipher = Crypt::CBC->new(
    -cipher		=> "Crypt::Rijndael",
    -padding		=> "standard",
    -header		=> "none",
    -literal_key	=> 1,
    -key		=> $aeskey,
    -iv			=> $aesiv,
    -blocksize		=> 16,
    );
  return $cipher->decrypt($ciphertext);
}

sub write_string_into_file
{
	my $fname = shift;
	$fname && open(__STRING, $fname) or return undef;
	for (my $i = 0; $i < @_; $i++) {
		if (ref($_[$i]) eq "SCALAR") {
			print __STRING ${$_[$i]} || return undef;
		} else {
			print __STRING $_[$i] || return undef;
		}
	}
	close(__STRING) || return undef;
	return 1;
}

sub read_file_into_string($)
{
        my $name = shift;
	my $result;

        return undef unless defined $name && $name;
        return undef unless open(__STRING, "$name");

        local $/=undef;
        $result = <__STRING>;
        close(__STRING);

	if ((!defined $result) && (!$!))
	{
		# 0 byte file - return empty string but no undef
		return "";
	}

        return $result;
}

sub tohex($)
{
  my ($in) = @_;
  my $out = "";
  my $c = 0;
  while ($c < length($in)) {
    $out .= sprintf("%02x", ord(substr($in, $c, 1)));
    $c++;
  }
  return $out;
}
