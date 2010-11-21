#
# /etc/cron.d/chaosvpn: crontab entries for the chaosvpn package
#

SHELL=/bin/bash
PATH=/usr/local/sbin:/usr/local/bin:/sbin:/bin:/usr/sbin:/usr/bin
MAILTO=root

# note:
# the $RANDOM below does not work with dash and needs bash!

23 4,16	* * *	root	[ -x /etc/init.d/chaosvpn ] && sleep $(( $RANDOM \% 900 )) && /usr/sbin/invoke-rc.d chaosvpn reload >/dev/null

# sleeps a random amount of seconds up to around 15 minutes after
# the cronjob starts to avoid all clients hitting the server at
# the same time
