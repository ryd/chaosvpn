#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "fs.h"
#include "list.h"
#include "main.h"
#include "tun.h"
#include "http.h"
#include "parser.h"
#include "tinc.h"
#include "settings.h"
#include "daemon.h"
#include "string/string.h"

int r_sigterm = 0;
int r_sigint = 0;
struct daemon_info di_tincd;

extern FILE *yyin;

/* getopt switches */
static char* CONFIG_FILE = "/etc/tinc/chaosvpn.conf";
static int DONOTFORK = 0;

static int main_check_root(void);
static int main_create_backup(struct config*);
static void main_free_parsed_info(struct config*);
static int main_init(struct config*);
static void main_initialize_config(struct config*);
static int main_parse_config(struct config*, struct string*);
static void main_parse_opts(int, char**);
static int main_request_config(struct config*, struct string*);
static void main_terminate_old_tincd(void);
static void main_unlink_pidfile(void);
static int main_write_config_hosts(struct config*);
static int main_write_config_tinc(struct config*);
static int main_write_config_up(struct config*);
static void sigchild(int);
static void sigterm(int);
static void sigint(int);
static void sigint_holdon(int);
static void usage(void);


int
main (int argc,char *argv[]) {
	struct config config;

	main_parse_opts(argc, argv);

	main_initialize_config(&config);

	int err = main_init(&config);
	if (err) return err;

	(void)fputs("Fetching information:", stdout);
	(void)fflush(stdout);

	struct string http_response;
	string_init(&http_response, 4096, 512);

	err = main_request_config(&config, &http_response);
	if (err) return err;

	err = main_parse_config(&config, &http_response);
	if (err) return err;

	string_free(&http_response);

	(void)fputs(".\n", stderr);

	(void)fputs("Backing up old configs:", stderr);
	(void)fflush(stderr);
	if (main_create_backup(&config)) {
		(void)fputs("unable to complete the backup.\n", stderr);
		return 1;
	}
	(void)fputs(".\n", stderr);

	if (main_write_config_tinc(&config)) return 1;
	if (main_write_config_hosts(&config)) return 1;
	if (main_write_config_up(&config)) return 1;

	main_free_parsed_info(&config);

	// if (main_create_pidfile()) return 1;

	if (DONOTFORK) {
		daemon_init(&di_tincd, s_tincd_bin, s_tincd_bin, "-n", s_networkname, NULL);
	} else {
		daemon_init(&di_tincd, s_tincd_bin, s_tincd_bin, "-n", s_networkname, "-D", NULL);
		signal(SIGTERM, sigterm);
		signal(SIGINT, sigint);
		signal(SIGCHLD, sigchild);
	}
	if (DONOTFORK) {
		main_terminate_old_tincd();
	} else {
		main_unlink_pidfile();
	}

	puts("\x1B[31;1mStarting tincd.\x1B[0m");
	if (daemon_start(&di_tincd)) {
		(void)fputs("\x1B[31;1merror: unable to run tincd.\x1B[0m\n", stderr);
		exit(1);
	}

	if (!DONOTFORK) {
		while(!r_sigterm && !r_sigint) {
			sleep(2);
		}
		puts("\x1B[31;1mTerminating tincd.\x1B[0m");
		signal(SIGTERM, SIG_IGN);
		signal(SIGINT, sigint_holdon);
		signal(SIGCHLD, SIG_IGN);
		daemon_stop(&di_tincd, 5);
	}

	daemon_free(&di_tincd);
	settings_free_all();

	return 0;
}

static void
main_terminate_old_tincd(void)
{
	int pidfile;
	char pidbuf[32];
	int len;
	long readpid;
	pid_t pid;
	

	pidfile = open(s_pidfile, O_RDONLY);
	if (pidfile == -1) {
		(void)fputs("notice: unable to open pidfile; assuming an old tincd is not running\n", stderr);
		return;
	}
	len = read(pidfile, pidbuf, 31);
	close(pidfile);
	pidbuf[len] = 0;
	readpid = strtol(pidbuf, NULL, 10);
	pid = (pid_t) readpid;
	(void)fprintf(stderr, "notice: sending SIGTERM to old tincd instance (%d).\n", pid);
	kill(pid, SIGTERM);
	sleep(2);
	if (kill(pid, SIGKILL) == 0) {
		(void)fputs("warning: tincd needed SIGKILL; unlinking its pidfile.\n", stderr);
		// SIGKILL succeeded; hence, we must manually unlink the old pidfile.
		main_unlink_pidfile();
	}
}

static void
main_parse_opts(int argc, char** argv)
{
	int c;

	opterr = 0;
	while ((c = getopt(argc, argv, "c:af")) != -1) {
		switch (c) {
		case 'c':
			CONFIG_FILE = optarg;
			break;

		case 'a':
			DONOTFORK = 1;
			break;

		case 'f':
			DONOTFORK = 0;
			break;;

		default:
			usage();
		}
	}
}

static void
usage(void)
{
	(void)fputs("chaosvpn - connect to the chaos vpn.\n"
	       "Usage: chaosvpn [OPTION...]\n\n"
	       "  -c FILE                 use this user configuration file\n"
	       "  -a                      do not control tincd; leave it to itself\n"
	       "  -f                      control tincd (default)\n"
	       "\n",
		stderr);
	exit(1);
}

static int
main_check_root() {
	return getuid() != 0;
}

static void
main_initialize_config(struct config* config) {
	config->peerid		= NULL;
	config->vpn_ip		= NULL;
	config->vpn_ip6		= NULL;
	config->networkname = NULL;
	config->tincd_bin	= "/usr/sbin/tincd";
	config->ip_bin		= "/sbin/ip";
	config->ifconfig	= "/sbin/ifconfig";
	config->ifconfig6	= NULL; // not required
	config->master_url	= "https://www.vpn.hamburg.ccc.de/tinc-chaosvpn.txt";
	config->base_path	= "/etc/tinc";
	config->pidfile		= "/var/run";
	config->my_peer		= NULL;

	INIT_LIST_HEAD(&config->peer_config);
}

static int
main_init(struct config *config) {
	struct stat st; 
	if (main_check_root()) {
		printf("Error - wrong user - please start as root user\n");
		return 1;
	}

	if (tun_check_or_create()) {
		printf("Error - unable to create tun device\n");
		return 1;
	}

	yyin = fopen(CONFIG_FILE, "r");
	if (!yyin) {
		(void)fprintf(stderr, "Error: unable to open %s\n", CONFIG_FILE);
		return 1;
	}
	yyparse();
	fclose(yyin);

	// optional params
	if (s_master_url != NULL)	config->master_url	= s_master_url;
	if (s_tincd_bin != NULL)	config->tincd_bin	= s_tincd_bin;
	if (s_ip_bin != NULL)		config->ip_bin		= s_ip_bin;
	if (s_ifconfig != NULL)		config->ifconfig	= s_ifconfig;
	if (s_base != NULL)			config->base_path	= s_base;
	if (s_pidfile != NULL)		config->pidfile		= s_pidfile;

	//require params
	if (s_my_peerid == NULL) {
		printf("peerid is missing in %s\n", CONFIG_FILE);
		return 1;
	}
	config->peerid = s_my_peerid;

	if (s_networkname == NULL) {
		printf("networkname is missing in %s\n", CONFIG_FILE);
		return 1;
	}
	config->networkname = s_networkname;

	if (stat(s_base, &st) & fs_mkdir_p(s_base, 0700)) {
		fprintf(stderr, "error: unable to mkdir %s\n", s_base);
		return 1;
	}

	return 0;
}

static int
main_request_config(struct config *config, struct string *http_response) {
	struct string httpurl;

	string_init(&httpurl, 512, 512);
	string_concat_sprintf(&httpurl, "%s?id=%s",
		config->master_url, config->peerid);

	if (http_request(string_get(&httpurl), http_response)) {
		printf("Unable to fetch %s - maybe server is down\n", config->master_url);
		string_free(&httpurl);
		return 1;
	}

	string_free(&httpurl);

	return 0;
}

static int
main_parse_config(struct config *config, struct string *http_response) {
	struct list_head *p = NULL;

	if (parser_parse_config(string_get(http_response), &config->peer_config)) {
		printf("Unable to parse config\n");
		return 1;
	}

	list_for_each(p, &config->peer_config) {
		struct peer_config_list *i = container_of(p,
				struct peer_config_list, list);
		if (strcmp(i->peer_config->name, config->peerid) == 0) {
				config->my_peer = i->peer_config;
		}
	}

	return 0;
}

static void
main_free_parsed_info(struct config* config)
{
	parser_free_config(&config->peer_config);
}

static int
main_write_config_tinc(struct config *config) {
	struct string configfilename;
	struct string tinc_config;

	(void)fputs("Writing global config file:", stdout);
	(void)fflush(stdout);
	if (string_init(&tinc_config, 8192, 2048)) return 1;
	if (tinc_generate_config(&tinc_config, config)) {
		string_free(&tinc_config);
		return 1;
	}

	string_init(&configfilename, 512, 512);
	string_concat(&configfilename, s_base);
	string_concat(&configfilename, "/tinc.conf");

	if (fs_writecontents(string_get(&configfilename), tinc_config.s,
			tinc_config._u._s.length, 0600)) {
		(void)fputs("unable to write tinc config file!\n", stderr);
		string_free(&tinc_config);
		string_free(&configfilename);
		return 1;
	}

	string_free(&tinc_config);
	string_free(&configfilename);

	(void)puts(".");

	return 0;
}

static int
main_write_config_hosts(struct config *config) {
	struct list_head *p = NULL;
	struct string hostfilepath;

	string_init(&hostfilepath, 512, 512);
	string_concat(&hostfilepath, s_base);
	string_concat(&hostfilepath, "/hosts/");

	fs_mkdir_p(string_get(&hostfilepath), 0700);

	list_for_each(p, &config->peer_config) {
		struct string peer_config;
		struct peer_config_list *i = container_of(p, 
				struct peer_config_list, list);

		if (string_init(&peer_config, 2048, 512)) return 1;

		printf("Writing config file for peer %s:", i->peer_config->name);
		(void)fflush(stdout);

		if (tinc_generate_peer_config(&peer_config, i->peer_config)) return 1;

		if(fs_writecontents_safe(string_get(&hostfilepath), 
				i->peer_config->name, peer_config.s, 
				peer_config._u._s.length, 0600)) {
			fputs("unable to write host config file.\n", stderr);
			string_free(&peer_config);
			return 1;
		}

		string_free(&peer_config);
		(void)puts(".");
	}

	string_free(&hostfilepath);
	
	return 0;
}

static int
main_write_config_up(struct config *config) {
	struct string up_filecontents;
	struct string up_filepath;

	string_init(&up_filecontents, 8192, 2048);
	tinc_generate_up(&up_filecontents, config);

	string_init(&up_filepath, 512, 512);
	string_concat(&up_filepath, config->base_path);
	string_concat(&up_filepath, "/tinc-up");
	if(fs_writecontents(string_get(&up_filepath), up_filecontents.s, up_filecontents._u._s.length, 0750)) {
		(void)fprintf(stderr, "unable to write to %s!\n", string_get(&up_filepath));
		string_free(&up_filecontents);
		string_free(&up_filepath);
		return 1;
	}
	
	string_free(&up_filecontents);
	string_free(&up_filepath);

	return 0;
}

static int
main_create_backup(struct config *config) {
	struct string base_backup_fn;

	if (string_init(&base_backup_fn, 512, 512)) return 1; /* don't goto bail_out here */
	if (string_concat(&base_backup_fn, config->base_path)) goto bail_out;
	if (string_concatb(&base_backup_fn, ".old", 5)) goto bail_out;

	if (fs_cp_r(config->base_path, string_get(&base_backup_fn))) goto bail_out;

	string_free(&base_backup_fn);

	return 0;

bail_out:
	string_free(&base_backup_fn);
	return 1;
}

static void
main_unlink_pidfile(void)
{
	(void)unlink(s_pidfile);
}

static void
sigchild(int __unused)
{
	fprintf(stderr, "\x1B[31;1mtincd terminated. Restarting in %d seconds.\x1B[0m\n", s_tincd_restart_delay);
	main_unlink_pidfile();
	if (daemon_sigchld(&di_tincd, s_tincd_restart_delay)) {
		fputs("\x1B[31;1munable to restart tincd. Terminating.\x1B[0m\n", stderr);
		exit(1);
	}
}

static void
sigterm(int __unused)
{
	r_sigterm = 1;
}

static void
sigint(int __unused)
{
	r_sigint = 1;
}

static void
sigint_holdon(int __unused)
{
	puts("I'm doing me best, please be patient for a little, will ya?");
}


