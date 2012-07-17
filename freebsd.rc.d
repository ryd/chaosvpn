#!/bin/sh
#

# PROVIDE: chaosvpn
# REQUIRE: ipfilter FILESYSTEMS sysctl netif tinc
# BEFORE:  SERVERS routing
# KEYWORD: nojail

# Define these chaosvpn_* variables in one of these files:
#       /etc/rc.conf
#       /etc/rc.conf.local
#       /etc/rc.conf.d/threeproxy
#
# DO NOT CHANGE THESE DEFAULT VALUES HERE

chaosvpn_enable=${chaosvpn_enable:-"NO"}

. /etc/rc.subr

name="chaosvpn"
rcvar="chaosvpn_enable"
command="/usr/local/sbin/chaosvpn"
start_precmd="chaosvpn_prestart"
start_cmd="chaosvpn_start"
#stop_cmd="chaosvpn_stop"
#reload_cmd="chaosvpn_reload"
#extra_commands="reload"
procname=${command:-chaosvpn}

load_rc_config $name

: ${chaosvpn_config:="/usr/local/etc/tinc/chaosvpn.conf"}
: ${chaosvpn_flags:="-d"}

chaosvpn_prestart()
{
	
}

chaosvpn_start()
{
	for cfg in ${chaosvpn_config}
	do
		echo "Starting ChaosVPN for ${cfg}"
		$command -d -c $cfg
	done
}

chaosvpn_stop()
{
	echo "**FIXME** chaosvpn_stop() to be implemented"
}

run_rc_command "$1"
# eof
