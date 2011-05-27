
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
	struct string lockfile;
	int fh_lockfile;
	FILE *handle_pidfile;
	bool retval = false;

	string_init(&lockfile, 512, 512);
	string_concat(&lockfile, filename);
	string_concat(&lockfile, ".lck");
	string_putc(&lockfile, 0);

	fh_lockfile = open(string_get(&lockfile), O_CREAT | O_EXCL, 0600);
	if (fh_lockfile == -1) {
		log_info("create_pidfile: lockfile already exists.");
		goto out_free;
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
	if (unlink(string_get(&lockfile))) {
		log_warn("create_pidfile: warning: couldn't remove lockfile %s", string_get(&lockfile));
	}
out_free:
	string_free(&lockfile);
	return retval;
}

