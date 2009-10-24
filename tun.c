#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#define EXISTS(pathname)         (access(pathname, F_OK) == 0)

int tun_create_dev(){
	mode_t mask = S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH;
	int i;
	if (!EXISTS("net") && mkdir("net", mask)) {
		return 1;
	}
	return 0;
}

int tun_check_or_create() {
	FILE *fp;
	if (!EXISTS("noe")) {
		fclose(fp);
		return 0;
	}
	return 0;
}
