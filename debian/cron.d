#
# /etc/cron.d/chaosvpn: crontab entries for the chaosvpn package
#
23 4,16	* * *	root	[ -x /etc/init.d/chaosvpn ] && sleep $[ $RANDOM / 36 ] && invoke-rc.d chaosvpn reload

# sleeps a random amount of seconds up to around 15 minutes after
# the cronjob starts to avoid all clients hitting the server at
# the same time
