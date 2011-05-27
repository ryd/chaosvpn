
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "chaosvpn.h"

bool
pidfile_create_pidfile(const char *filename)
{
	struct string lockfile;
	int fh_lockfile;
	int fh_pidfile;
	char pidbuf[64];
	int len;
	bool retval = false;

	string_init(&lockfile, 512, 512);
	string_concat(&lockfile, filename);
	string_concat(&lockfile, ".lck");
	string_putc(&lockfile, 0);

	fh_lockfile = open(string_get(&lockfile), O_CREAT | O_EXCL, 0600);
	if (fh_lockfile == -1) {
		log_info("create_pidfile: lockfile already exists.");
		return false;
	}
	close(fh_lockfile);

	// There's a better way to convert an int into a string.
	len = snprintf(pidbuf, 64, "%d", getpid());
	fh_pidfile = open(filename, O_CREAT | O_WRONLY | O_TRUNC, 0600);
	if (write(fh_pidfile, pidbuf, len) != len)
		goto bail_out;

	retval = true;

bail_out:
	close(fh_pidfile);
	if (unlink(string_get(&lockfile))) {
		log_warn("create_pidfile: warning: couldn't remove lockfile %s", string_get(&lockfile));
	}
	return retval;
}

