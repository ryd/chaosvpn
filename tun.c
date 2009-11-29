#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/sysmacros.h>

#include "tun.h"

#define EXISTS(pathname)         (access(pathname, F_OK) == 0)
#define NOERR   (0)

static int tun_create_dev(){
	mode_t mask = S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH;
	if (!EXISTS(TUN_PATH) && (NOERR != mkdir(TUN_PATH, mask))) {
		return 1;
	}
	if (NOERR != mknod (TUN_DEV, S_IFCHR | S_IRUSR | S_IWUSR, makedev(10, 200))) {
		return 1;
	}
	return 0;
}

int tun_check_or_create() {
	if (!EXISTS(TUN_DEV)) {
		printf("tun device does't exists - trying to create it.");
		return tun_create_dev();
	}
	return 0;
}
