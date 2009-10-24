#include <stdlib.h>
#include <stdio.h>

int main (int argc,char *argv[]) {
		if (tun_check_or_create()) {
			printf("Error - unable to create tun device - maybe wrong user\n");
			return 1;
		}

		char buffer[8196];
		if (http_request("http://nons.de/", buffer)) {
			printf("somthing went wrong\n");
			return 1;
		}
		printf("http:%s\n", buffer);
		return 0;
}
