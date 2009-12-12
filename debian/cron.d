#
# Regular cron jobs for the chaosvpn package
#
0 4,11	* * *	root	[ -x /etc/init.d/chaosvpn ] && invoke-rc.d chaosvpn restart

# TODO: do not run at a fixed some, add some random to avoid
#       all clients hitting the server at the same time
