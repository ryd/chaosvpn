Name:		chaosvpn
Version:	2.14
Release:	1fbo%{?dist}
Summary:	ChaosVPN is a system to connect Hackers.
Packager:       Frank.Botte@web.de
Group:		Applications/Communications
License:	Apache License Version 2.0
URL:		http://wiki.hamburg.ccc.de/ChaosVPN
Source0:	chaosvpn.tgz
BuildRoot:	%(mktemp -ud %{_tmppath}/%{name}-%{version}-%{release}-XXXXXX)

BuildRequires:	byacc, openssl-devel
Requires:	tinc >= 1.0.13

%description
ChaosVPN is a system to connect Hackers.

Design principals include that it should be without Single Point of Failure,
make usage of full encryption, use RFC1918 ip ranges, scales well on >100
connected networks and is being able to run on a embedded hardware you will
find in our todays router.

It should be designed that noone sees other peoples traffic.

It should be mainly autoconfig as in that besides the joining node no
administrator of the network should be in the need to acutally do something when
a node joins or leaves.

If you want to find a solution for a Network without Single Point of failure,
has - due to Voice over IP - low latency and that noone will see other peoples
traffic you end up pretty quick with a full mesh based network.

Therefore we came up with the tinc solution. tinc does a fully meshed peer to
peer network and it defines endpoints and not tunnels.

ChaosVPN connects hacker wherever they are. We connect roadwarriors with their
notebook. Servers, even virtual ones in Datacenters, Hackerhouses and
hackerspaces. To sum it up we connect networks - maybe down to a small /32.

So there we are. ChaosVPN is working and it seems the usage increases, more
nodes join in and more sevices pop up.

For now, an overview picture of the currently connected nodes:
http://vpnhub1-intern.hamburg.ccc.de/chaosvpn.png
(from inside ChaosVPN; updated once per hour), and a simple display of
node-uptimes: http://vpnhub1-intern.hamburg.ccc.de/chaosvpn.nodes.html. 

%prep
%setup -q -n chaosvpn

%build
make %{?_smp_mflags}

%install
rm -rf %{buildroot}
mkdir -p %{buildroot}/%{_initrddir} 
cp -p debian/init.d %{buildroot}%{_initrddir}/chaosvpn
mkdir -p %{buildroot}/%{_sysconfdir}/tinc/chaos
make install DESTDIR=%{buildroot}

%clean
rm -rf %{buildroot}

%post 
chkconfig --add chaosvpn

%preun 
chkconfig --del choasvpn

%files
%defattr(-,root,root,-)
%doc README NEWS  LICENCE INSTALL INSTALL.Windows Documentation/* contrib
%dir %{_sysconfdir}/tinc/chaos
%attr(0755, root, root) %{_initrddir}/chaosvpn
%config(noreplace) %{_sysconfdir}/tinc/chaosvpn.conf
%config(noreplace) %{_sysconfdir}/tinc/warzone.conf
%{_sbindir}/chaosvpn
%doc %{_mandir}/man1/chaosvpn.1.gz
%doc %{_mandir}/man5/chaosvpn.conf.5.gz

%changelog
* Sun Sep 16 2012 Frank Botte <Frank.Botte@web.de> 2.14-1
- corrected version number
- git pulled on 09092012 
* Sat Sep 15 2012 Frank Botte <Frank.Botte@web.de> 2.0-1
- first public RPM...
 
