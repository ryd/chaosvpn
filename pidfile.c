/* UNUSED! */

#include "log.h"

int
pidfile_create_pidfile()
{
	struct string lockfile;
	int fh_lockfile;
	int fh_pidfile;
	char pidbuf[64];
	int len;
	int retval = 0;

	if (str_is_empty(s_pidfile) {
		log_debug("create_pidfile: no PIDFILE is set.");
		return 2;
	}

	string_init(&lockfile, 512, 512);
	string_concat(&lockfile, s_pidfile);
	string_concat(&lockfile, ".lck");
    
	fh_lockfile = open(string_get(&lockfile), O_CREAT | O_EXCL, 0600);
	if (fh_lockfile == -1) {
		log_info("create_pidfile: lockfile already exists.");
		return 111;
	}
	close(fh_lockfile);

	// There's a better way to convert an int into a string.
	len = snprintf(pidbuf, 64, "%d", getpid());
	fh_pidfile = open(s_pidfile, O_CREAT | O_WRONLY | O_TRUNC, 0600);
	if (write(fh_pidfile, pidbuf, len) != len) {
		retval = 1;
		goto bail_out;
	}

bail_out:
	close(fh_pidfile);
	if (unlink(string_get(&lockfile))) {
		log_warn("create_pidfile: warning: couldn't remove lockfile %s", string_get(&lockfile));
	}
	return retval;
}

