#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "chaosvpn.h"

#define EXISTS(pathname)         (access(pathname, F_OK) == 0)
#define NOERR   (0)         

unsigned int
gnu_dev_major (unsigned long long int dev)
{
  return ((dev >> 8) & 0xfff) | ((unsigned int) (dev >> 32) & ~0xfff);
}

unsigned int
gnu_dev_minor (unsigned long long int dev)
{
  return (dev & 0xff) | ((unsigned int) (dev >> 12) & ~0xff);
}

unsigned long long int
gnu_dev_makedev (unsigned int major, unsigned int minor)
{
  return ((minor & 0xff) | ((major & 0xfff) << 8)
          | (((unsigned long long int) (minor & ~0xff)) << 12)
          | (((unsigned long long int) (major & ~0xfff)) << 32));
}

static bool
tun_create_dev(void)
{
	mode_t mask = S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH;
	if (!EXISTS(TUN_PATH) && (NOERR != mkdir(TUN_PATH, mask))) {
		return false;
	}
	if (NOERR != mknod (TUN_DEV, S_IFCHR | S_IRUSR | S_IWUSR, gnu_dev_makedev(10, 200))) {
		return false;
	}
	return true;
}

bool
tun_check_or_create(void)
{
	if (!EXISTS(TUN_DEV)) {
		log_info("tun device %s does't exists - trying to create it.", TUN_DEV);
		return tun_create_dev();
	}
	return true;
}
