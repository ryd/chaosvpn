#include <sys/types.h>
#include <unistd.h>

#include "chaosvpn.h"

void
nonroot(struct config* config)
{
    if (setegid(config->tincd_gid) |
        seteuid(config->tincd_uid)) {
        log_err("Unable to seteuid/setegid to %d/%d.", config->tincd_uid, config->tincd_gid);
        exit(1);
    }
}

void
root(void)
{
    if (seteuid(0) |
        setegid(0)) {
        log_err("Unable to seteuid/setegid to 0/0.");
        exit(1);
    }
}
