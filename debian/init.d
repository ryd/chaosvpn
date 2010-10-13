#!/bin/sh
#
### BEGIN INIT INFO
# Provides:          chaosvpn
# Required-Start:    $network $local_fs $remote_fs
# Required-Stop:     $network $local_fs $remote_fs
# Should-Start:      $named
# Should-Stop:
# Default-Start:     2 3 4 5
# Default-Stop:      0 1 6
# Short-Description: <Enter a short description of the sortware>
# Description:       <Enter a long description of the software>
#                    <...>
#                    <...>
### END INIT INFO

PATH=/usr/local/sbin:/usr/local/bin:/sbin:/bin:/usr/sbin:/usr/bin

DAEMON=/usr/sbin/chaosvpn # Introduce the server's location here
TINCCTL=/usr/sbin/tincctl
NAME=chaosvpn             # Introduce the short server's name here
DESC="Agora Link"         # Introduce a short description here

PIDFILE="overridden later"


test -x $DAEMON || exit 0

. /lib/lsb/init-functions

# Default options, these can be overriden by the information
# at /etc/default/$NAME
DAEMON_OPTS="-o"        # Additional options given to the server
# -o == oneshot mode, do not run as daemon, onetime fetch+update of config
#       and tincd restart

DIETIME=10              # Time to wait for the server to die, in seconds
                        # If this value is set too low you might not
                        # let some servers to die gracefully and
                        # 'restart' will not work

STARTTIME=1             # Time to wait for the server to start, in seconds
                        # If this value is set each time the server is
                        # started (on start or restart) the script will
                        # stall to try to determine if it is running
                        # If it is not set and the server takes time
                        # to setup a pid file the log message might
                        # be a false positive (says it did not start
                        # when it actually did)

DAEMONUSER=""           # Users to run the daemons as. If this value
                        # is set start-stop-daemon will chuid the server

CONFIGS="chaosvpn.conf"	# which network config to startup


# Include defaults if available
if [ -f /etc/default/$NAME ] ; then
    . /etc/default/$NAME
fi

# Use this if you want the user to explicitly set 'RUN' in
# /etc/default/
if [ "x$RUN" != "xyes" ] ; then
    log_failure_msg "$NAME disabled, please adjust the configuration to your needs "
    log_failure_msg "and then set RUN to 'yes' in /etc/default/$NAME to enable it."
    exit 0
fi

# Check that the user exists (if we set a user)
# Does the user exist?
if [ -n "$DAEMONUSER" ] ; then
    if getent passwd | grep -q "^$DAEMONUSER:"; then
        # Obtain the uid and gid
        DAEMONUID=`getent passwd |grep "^$DAEMONUSER:" | awk -F : '{print $3}'`
        DAEMONGID=`getent passwd |grep "^$DAEMONUSER:" | awk -F : '{print $4}'`
    else
        log_failure_msg "The user $DAEMONUSER, required to run $NAME does not exist."
        exit 0
    fi
fi


running_pid() {
# Check if a given process pid's cmdline matches a given name
    local pid=$1
    local name=$2
    [ -z "$pid" ] && return 1
    [ ! -d /proc/$pid ] &&  return 1
    local cmd=`cat /proc/$pid/cmdline | tr "\000" "\n"|head -n 1 |cut -d : -f 1`
    # Is this the expected server
    [ "$cmd" != "$name" ] &&  return 1
    return 0
}

start_server() {
	local errcode
# Start the process using the wrapper
        if [ -z "$DAEMONUSER" ] ; then
            start_daemon -p $PIDFILE $DAEMON $DAEMON_OPTS >/dev/null
            errcode=$?
        else
# if we are using a daemonuser then change the user id
            start-stop-daemon --start --quiet --pidfile $PIDFILE \
                        --chuid $DAEMONUSER \
                        --exec $DAEMON -- $DAEMON_OPTS
            errcode=$?
        fi
        return $errcode
}

extract_configinfos() {
	local config="$1"
	PIDFILE=""
	DAEMON_OPTS="--help"
	networkname=""
	
	[ -f "/etc/tinc/$config" ] || return 1

	# TODO: replace the following with something smaller like sed/busybox sed
	networkname=$( perl -ne 's/^.*?\$networkname\s*=\s*\"(.*?)\".*$/$1/ && print $_' <"/etc/tinc/$config" )
	[ -z "$networkname" ] && return 1

	PIDFILE="/var/run/tinc.$networkname.pid"
	DAEMON_OPTS="-a -c /etc/tinc/$config"
	return 0;
}

start_configs() {
	local mode="$1"
	local errcode
	local config
	for config in $CONFIGS ; do
		if [ "$mode" = "reload" ] ; then
			log_daemon_msg "Reloading $DESC " "$config"
		else
			log_daemon_msg "Starting $DESC " "$config"
		fi
		errcode=1
		
		if extract_configinfos "$config" ; then
			start_server
			errcode=$?

			if [ "$errcode" -eq 0 ] ; then
				[ -n "$STARTTIME" ] && sleep $STARTTIME # Wait some time
		
				if [ -f "$PIDFILE" ] ; then
					# check if tincd is running
					pidofproc -p "$PIDFILE" /usr/sbin/tincd >/dev/null || errcode=1
				fi
			fi
		fi

		log_end_msg $errcode
	done
}

stop_configs() {
	local config
	for config in $CONFIGS ; do
		log_daemon_msg "Stopping $DESC " "$config"
		extract_configinfos "$config"
		if [ -x "$TINCCTL" ] ; then
			# tincctl exists since tinc 1.1-git
			"$TINCCTL" -n "$networkname" stop 2>/dev/null
			log_end_msg $?
		else
			if [ -f "$PIDFILE" ] ; then
				killproc -p $PIDFILE /usr/sbin/tincd
				log_end_msg $?
			else
				log_progress_msg "apparently not running"
				log_end_msg 0
			fi
		fi
	done
	return 0
}


case "$1" in
  start)
	start_configs
        ;;
  stop)
	stop_configs
        ;;
  force-stop)
	stop_configs
        ;;
  restart|force-reload)
	stop_configs && start_configs
        ;;
#  status)
#        log_daemon_msg "Checking status of $DESC" "$NAME"
#        if running ;  then
#            log_progress_msg "running"
#            log_end_msg 0
#        else
#            log_progress_msg "apparently not running"
#            log_end_msg 1
#            exit 0
#        fi
#        ;;
  reload)
        #log_warning_msg "Reloading $NAME daemon: not implemented, as the daemon"
        #log_warning_msg "cannot re-read the config file (use restart)."
        # Reloading is the same as starting for us
        start_configs "reload"
        ;;
  *)
        N=/etc/init.d/$NAME
        echo "Usage: $N {start|stop|force-stop|restart|force-reload|status}" >&2
        exit 1
        ;;
esac

exit 0
