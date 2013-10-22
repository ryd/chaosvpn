#include <string.h>
#include <sys/socket.h>
#ifndef WIN32
#include <arpa/inet.h>
#endif
#include <netinet/in.h>
#include <netdb.h>

#include "chaosvpn.h"
#include "addrmask.h"

struct addr_info ip;
struct string str;
bool ret;

int
main (int argc,char *argv[])
{
	log_init(&argc, &argv, LOG_PID, LOG_DAEMON);

	log_info("test_addrmask started.\n");

	/* IPV4 */

	ret = addrmask_parse(&ip, "127.0.0.1/8");
	if (!ret) {
		log_err("addrmask_parse(&ip, \"127.0.0.1/8\") failed.\n");
		exit(1);
	}

	log_info("ip addr_family = %d, (v4=%d, v6=%d)\n", ip.addr_family, AF_INET, AF_INET6);

	string_init(&str, 128, 128);
	ret = addrmask_to_string(&str, &ip);
	if (!ret) {
		log_err("addrmask_to_string(&str, &ip) failed.\n");
		exit(1);
	}

	string_ensurez(&str);
	log_info("addrmask_to_string(): %s\n", string_get(&str));
	string_free(&str);

	/* IPV6 */

	ret = addrmask_parse(&ip, "fe80::227:13ff:feb5:160e/64");
	if (!ret) {
		log_err("addrmask_parse(&ip, \"fe80::227:13ff:feb5:160e/64\") failed.\n");
		exit(1);
	}

	log_info("ip addr_family = %d, (v4=%d, v6=%d)\n", ip.addr_family, AF_INET, AF_INET6);

	string_init(&str, 128, 128);
	ret = addrmask_to_string(&str, &ip);
	if (!ret) {
		log_err("addrmask_to_string(&str, &ip) failed.\n");
		exit(1);
	}

	string_ensurez(&str);
	log_info("addrmask_to_string(): %s\n", string_get(&str));
	string_free(&str);

	log_info("test_addrmask finished.\n");
	exit(0);
}
