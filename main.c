#include <stdlib.h>
#include <stdio.h>

int main (int argc,char *argv[]) {
		if (tun_check_or_create()) {
			printf("/dev/net/tun missing - creating it\n");
		}
		return 0;
}
