#include <stdlib.h>
#include <string.h>

// NOTE: *must* assign NULL here to string values; initialize those in
// settings_init_defaults(). All numerics may be initialized here.

char* s_my_peerid = NULL;
char* s_my_vpn_ip = NULL;
char* s_my_vpn_ip6 = NULL;
char* s_my_password = NULL;
char* s_my_ip = NULL;
char* s_networkname = NULL;
char* s_tincd_bin = NULL;
char* s_routeadd = NULL;
char* s_routeadd6 = NULL;
char* s_ifconfig = NULL;
char* s_ifconfig6 = NULL;
char* s_master_url = NULL;
char* s_base = NULL;
char* s_configdata_signkey = NULL;
char* s_pidfile = NULL;
char* s_interface = NULL;
char* s_my_vpn_netmask = NULL;
int s_tincd_debuglevel = 3;
int s_tincd_restart_delay = 20;


// Note: all settings *must* be strdupped!
void
settings_init_defaults(void)
{
    s_interface = strdup("eth0");
}

void
settings_free_all(void)
{
	free(s_my_peerid);
	free(s_my_vpn_ip);
	free(s_my_vpn_ip6);
	free(s_my_password);
	free(s_my_ip);
	free(s_networkname);
	free(s_tincd_bin);
	free(s_routeadd);
	free(s_routeadd6);
	free(s_ifconfig);
	free(s_ifconfig6);
	free(s_master_url);
	free(s_base);
	free(s_configdata_signkey);
	free(s_pidfile);
	free(s_interface);
	free(s_my_vpn_netmask);
}
