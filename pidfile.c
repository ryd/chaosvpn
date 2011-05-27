
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>

#include "chaosvpn.h"

bool
pidfile_create_pidfile(const char *filename)
{
	char lockfile[512];
	int fh_lockfile;
	FILE *handle_pidfile;
	bool retval = false;

	snprintf(lockfile, sizeof(lockfile), "%s.lck", filename);

	fh_lockfile = open(lockfile, O_CREAT | O_EXCL, 0600);
	if (fh_lockfile == -1) {
		log_info("create_pidfile: lockfile '%s' already exists.", lockfile);
		goto out;
	}
	close(fh_lockfile);

	handle_pidfile = fopen(filename, "w");
	if (!handle_pidfile) {
		log_err("create_pidfile: error creating pidfile: %s", strerror(errno));
		goto out_unlock;
	}
	if (fprintf(handle_pidfile, "%d\n", getpid()) < 0) {
		log_err("create_pidfile: error writing to pidfile: %s", strerror(errno));
		goto out_close;
	}

	if (fclose(handle_pidfile) != 0) {
		log_err("create_pidfile: error writing to pidfile: %s", strerror(errno));
		goto out_unlock;
	}

	retval = true;
	goto out_unlock;

out_close:
	fclose(handle_pidfile);
out_unlock:
	if (unlink(lockfile)) {
		log_warn("create_pidfile: warning: couldn't remove lockfile '%s': %s", lockfile, strerror(errno));
	}
out:
	return retval;
}

