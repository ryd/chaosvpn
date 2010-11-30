#include <sys/param.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "chaosvpn.h"

static int r_sigterm = 0;
static int r_sigint = 0;
static struct daemon_info di_tincd;

static time_t nextupdate = 0;
static struct string HTTP_USER_AGENT;


static int main_check_root(void);
static int main_create_backup(struct config*);
static int main_cleanup_hosts_subdir(struct config*);
static int main_fetch_and_apply_config(struct config* config, struct string* oldconfig);
static void main_free_parsed_info(struct config*);
static int main_load_previous_config(struct config*, struct string*);
static int main_parse_config(struct config*, struct string*);
static void main_parse_opts(struct config*, int, char**);
static int main_request_config(struct config*, struct string*);
static void main_tempsave_fetched_config(struct config*, struct string*);
static void main_terminate_old_tincd(struct config*);
static void main_unlink_pidfile(struct config*);
static void main_updated(struct config*);
static void sigchild(int);
static void sigterm(int);
static void sigint(int);
static void sigint_holdon(int);
static void usage(void);
static void main_warn_about_old_tincd(struct config* config);

int
main (int argc,char *argv[])
{
	struct config *config;
	char tincd_debugparam[32];
	int err;
	struct string oldconfig;

	string_init(&HTTP_USER_AGENT, 64, 16);
	string_concat_sprintf(&HTTP_USER_AGENT, "ChaosVPN/%s", VERSION);

	log_init(&argc, &argv, LOG_PID, LOG_DAEMON);

	log_info("ChaosVPN/AgoraLink client v%s starting.", VERSION);

	crypto_warn_openssl_version_changed();

	config = config_alloc();
	if (config == NULL) {
		log_err("config malloc error\n");
		exit(EXIT_FAILURE);
	}

	main_parse_opts(config, argc, argv);

	if (main_check_root()) {
		log_err("Error - wrong user - please start as root user\n");
		return 1;
	}

#ifndef BSD
	if (!tun_check_or_create()) {
		log_err("Error - unable to create tun device\n");
		return 1;
	}
#endif

	err = config_init(config);
	if (err) return err;

	/* At this point, we've read and parsed our config file. */

	unroot(config);

	string_init(&oldconfig, 4096, 4096);
	main_fetch_and_apply_config(config, &oldconfig);

	main_warn_about_old_tincd(config);

	snprintf(tincd_debugparam, sizeof(tincd_debugparam), "--debug=%u", config->tincd_debuglevel);

	if (config->daemonmode) {
		if (!daemonize()) {
			log_err("daemonizing into the background failed - aborting\n");
			exit(1);
		}
	}
		
	if (config->oneshot) {
		daemon_init(&di_tincd, config->tincd_bin, config->tincd_bin, "-n", config->networkname, tincd_debugparam, NULL);
	} else {
		daemon_init(&di_tincd, config->tincd_bin, config->tincd_bin, "-n", config->networkname, tincd_debugparam, "-D", NULL);
		(void)signal(SIGTERM, sigterm);
		(void)signal(SIGINT, sigint);
		(void)signal(SIGCHLD, sigchild);
	}
	if (config->tincd_user) {
		daemon_addparam(&di_tincd, "--user");
		daemon_addparam(&di_tincd, config->tincd_user);
	}

	main_updated(config);

	root();
	main_terminate_old_tincd(config);
	log_info("Starting tincd.");
	if (!daemon_start(&di_tincd)) {
		log_err("error: unable to run tincd.");
		exit(EXIT_FAILURE);
	}
	unroot(config);

	if (!config->oneshot) {
		do {
		ml_cont:
			main_updated(config);
			while (1) {
				(void)sleep(2);
				if (nextupdate < time(NULL)) {
					break;
				}
				if (r_sigterm || r_sigint) {
					goto bail_out;
				}
			}
			switch (main_fetch_and_apply_config(config, &oldconfig)) {
			case -1:
				log_err("Error while updating config. Not terminating tincd.");
				goto ml_cont;

			case 1:
				log_info("No update needed.");
				goto ml_cont;

			default:;
			}
			log_info("Terminating old tincd.");
			daemon_stop(&di_tincd, 5);
		} while (!r_sigterm && !r_sigint);

		bail_out:
		log_info("Terminating old tincd.");
		(void)signal(SIGTERM, SIG_IGN);
		(void)signal(SIGINT, sigint_holdon);
		(void)signal(SIGCHLD, SIG_IGN);
		daemon_stop(&di_tincd, 5);
	}

	daemon_free(&di_tincd);
	string_free(&oldconfig);
	free(config); config = NULL;

	return 0;
}

static void
main_updated(struct config *config)
{
    nextupdate = time(NULL) + config->update_interval;
}

static int
main_fetch_and_apply_config(struct config* config, struct string* oldconfig)
{
	int err;
	struct string http_response;

	log_debug("Fetching information.");

	string_init(&http_response, 4096, 512);

	err = main_request_config(config, &http_response);
	if (err) {
		string_free(&http_response);
		if (main_load_previous_config(config, &http_response)) {
			return err;
		}
		log_warn("Warning: Unable to fetch config; using last stored config.");
	}

	if (string_equals(&http_response, oldconfig) == 0) {
		string_free(&http_response);
		return 1;
	}

	err = main_parse_config(config, &http_response);
	if (err) {
		string_free(&http_response);
		return -1;
	}

	// tempsave new config
	main_tempsave_fetched_config(config, &http_response);

	/* replaced by string_move */
	// string_free(&http_response);
	string_free(oldconfig);
	string_move(&http_response, oldconfig);

	root();
	log_debug("Backing up old configs.");
	if (main_create_backup(config)) {
		log_err("Unable to complete config backup.");
		return -1;
	}

	log_debug("Cleanup previous host entries.");
	if (main_cleanup_hosts_subdir(config)) {
		log_err("Unable to remove previous host subconfigs.");
		return -1;
	}

	if (!tinc_write_config(config)) return -1;
	if (!tinc_write_hosts(config)) return -1;
	if (!tinc_write_updown(config, true)) return -1;
	if (!tinc_write_updown(config, false)) return -1;
	if (!tinc_write_subnetupdown(config, true)) return -1;
	if (!tinc_write_subnetupdown(config, false)) return -1;
	unroot(config);

	main_free_parsed_info(config);

	return 0;
}

static void
main_terminate_old_tincd(struct config *config)
{
	pid_t pid;
	
	pid = tinc_get_pid(config);
	if (pid == 0)
		return;

	log_info("Notice: sending SIGTERM to old tincd instance (%d).", pid);
	(void)kill(pid, SIGTERM);
	(void)sleep(2);
	if (kill(pid, SIGKILL) == 0) {
		log_warn("Warning: tincd needed SIGKILL; unlinking its pidfile.\n");
		// SIGKILL succeeded; hence, we must manually unlink the old pidfile.
		main_unlink_pidfile(config);
	}
}

static void
main_parse_opts(struct config *config, int argc, char** argv)
{
	int c;

	opterr = 0;
	while ((c = getopt(argc, argv, "c:aofrd")) != -1) {
		switch (c) {
		case 'c':
			config->configfile = optarg;
			break;

		case 'a': /* legacy */
		case 'o':
			config->daemonmode = false;
			config->oneshot = true;
			break;

		case 'f': /* legacy */
		case 'r':
			config->daemonmode = false;
			config->oneshot = false;
			break;

		case 'd':
			config->daemonmode = true;
			config->oneshot = false;
			break;

		default:
			usage();
		}
	}
}

static void
usage(void)
{
	fprintf(stderr, "chaosvpn - connect to the chaos vpn.\n"
	       "Usage: chaosvpn [OPTION...]\n\n"
	       "  -c FILE  use this user configuration file\n"
	       "  -o       oneshot config update and tincd restart, then exit\n"
	       "  -r       keep running, controlling tincd (default)\n"
	       "  -d       daemon mode, fork into background and keep running\n"
	       "\n");
	exit(EXIT_FAILURE);
}

static void
main_warn_about_old_tincd(struct config* config)
{
	if (config->tincd_version &&
		(strnatcmp(config->tincd_version, "1.0.12") <= 0)) {
		log_warn("Warning: Old tinc version '%s' detected, consider upgrading!\n", config->tincd_version);
	}
}

static int
main_check_root()
{
	return getuid() != 0;
}

static int
main_request_config(struct config *config, struct string *http_response)
{
	int retval = 1;
	int httpres;
	struct string httpurl;
	struct string archive;
	struct string chaosvpn_version;
	struct string signature;
	struct string compressed;
	struct string encrypted;
	struct string rsa_decrypted;
	struct string aes_key;
	struct string aes_iv;
	char *buf;
	time_t startfetchtime;

	startfetchtime = time(NULL);

	/* fetch main configfile */

	crypto_init();

	/* basic string inits first, makes for way easier error-cleanup */
	string_init(&httpurl, 512, 128);
	string_init(&archive, 8192, 8192);
	string_lazyinit(&chaosvpn_version, 16);
	string_lazyinit(&signature, 1024);
	string_lazyinit(&compressed, 8192);
	string_lazyinit(&encrypted, 8192);
	string_lazyinit(&rsa_decrypted, 1024);
	string_lazyinit(&aes_key, 64);
	string_lazyinit(&aes_iv, 64);

	string_concat_sprintf(&httpurl, "%s?id=%s",
		config->master_url, config->peerid);

	if ((retval = http_get(&httpurl, &archive, config->ifmodifiedsince, &HTTP_USER_AGENT, &httpres, NULL))) {
		if (retval == HTTP_ESRVERR) {
			if (httpres == 304) {
				log_info("Not fetching %s - got HTTP %d - not modified\n", config->master_url, httpres);
				retval = 1;
			} else {
				log_info("Unable to fetch %s - got HTTP %d\n", config->master_url, httpres);
			}
		} else if (retval == HTTP_EINVURL) {
			log_err("\x1B[41;37;1mInvalid URL %s. Only http:// is supported.\x1B[0m\n", config->master_url);
			exit(1);
		} else {
			log_warn("Unable to fetch %s - maybe server is down\n", config->master_url);
		}
		goto bail_out;
	}
	config->ifmodifiedsince = startfetchtime;


	/* check if we received a new-style ar archive */
	if (!ar_is_ar_file(&archive)) {
		/* if we do not expect a signature than we can still use it
		   as the raw config (for debugging) */

		if (str_is_empty(config->masterdata_signkey)) {
			/* move whole received data to http_response */
			/* free old contents from http_response first */
			string_free(http_response);
			string_move(&archive, http_response);

			retval = 0; /* no error */
		} else {
			log_err("Invalid data format received from %s\n", config->master_url);
		}
		goto bail_out;
	}


	/* check chaosvpn-version in received ar archive */
	if (!ar_extract(&archive, "chaosvpn-version", &chaosvpn_version)) {
		string_free(&chaosvpn_version);
		log_err("chaosvpn-version missing - can't work with this config\n");
		goto bail_out;
	}
	string_putc(&chaosvpn_version, '\0');
	if (strcmp(string_get(&chaosvpn_version), "3") != 0) {
		string_free(&chaosvpn_version);
		log_err("unusable data-version from backend, we only support version 3!\n");
		goto bail_out;
	}
	string_free(&chaosvpn_version);

	if (str_is_empty(config->masterdata_signkey)) {
		/* no public key defined, nothing to verify against or to decrypt with */
		/* expect cleartext part */

		if (!ar_extract(&archive, "cleartext", http_response)) {
			log_err("cleartext part missing - can't work with this config\n");
			goto bail_out;
		}

		/* return success */
		retval = 0;
		goto bail_out;
	}


	/* get and decrypt rsa data block */
	if (!ar_extract(&archive, "rsa", &encrypted)) {
		log_err("rsa part in data from %s missing\n", config->master_url);
		goto bail_out;
	}
	if (crypto_rsa_decrypt(&encrypted, string_get(&config->privkey), &rsa_decrypted)) {
		log_err("rsa decrypt failed\n");
		goto bail_out;
	}
	string_free(&encrypted);

	/* check and copy data decrypted from rsa block */
	/* structure:
		1 byte length aes key
		1 byte length aes iv
		x bytes aes key
		y bytes aes iv
	   */
	if (string_length(&rsa_decrypted) < 2) {
		log_err("rsa decrypt result too short\n");
		goto bail_out;
	}
	buf = string_get(&rsa_decrypted);
	if (buf[0] != 32) {
		log_err("invalid aes keysize - expected 32, received %d\n", buf[0]);
		goto bail_out;
	}
	if (buf[1] != 16) {
		log_err("invalid aes ivsize - expected 16, received %d\n", buf[1]);
		goto bail_out;
	}
	if (string_length(&rsa_decrypted) < buf[0]+buf[1]+2) {
		log_err("rsa decrypt result too short\n");
		goto bail_out;
	}
	string_concatb(&aes_key, buf+2, buf[0]);
	string_concatb(&aes_iv, buf+2+buf[0], buf[1]);

	/* get, decrypt and uncompress config data */
	if (!ar_extract(&archive, "encrypted", &encrypted)) {
		log_err("encrypted data part in data from %s missing\n", config->master_url);
		goto bail_out;
	}
	if (crypto_aes_decrypt(&encrypted, &aes_key, &aes_iv, &compressed)) {
		log_err("data decrypt failed\n");
		goto bail_out;
	}
	string_free(&encrypted);
	if (!uncompress_inflate(&compressed, http_response)) {
		log_err("data uncompress failed\n");
		goto bail_out;
	}
	string_free(&compressed);

	/* get and decrypt signature */
	if (!ar_extract(&archive, "signature", &encrypted)) {
		log_err("signature part in data from %s missing\n", config->master_url);
		goto bail_out;
	}
	if (crypto_aes_decrypt(&encrypted, &aes_key, &aes_iv, &signature)) {
		log_err("signature decrypt failed\n");
		goto bail_out;
	}
	string_free(&encrypted);

	/* verify signature */
	retval = crypto_rsa_verify_signature(http_response, &signature, config->masterdata_signkey);


bail_out:
	/* free all strings, even if we may already freed them above */
	/* double string_free() is ok, and the error cleanup this way is easier */
	string_free(&httpurl);
	string_free(&archive);
	string_free(&chaosvpn_version);
	string_free(&signature);
	string_free(&compressed);
	string_free(&encrypted);
	string_free(&rsa_decrypted);
	string_free(&aes_key);
	string_free(&aes_iv);

	// make sure result is null-terminated
	// ar_extract() and crypto_*_decrypt() do not guarantee this!
	string_putc(http_response, '\0');

	crypto_finish();

	return retval;
}

static int
main_parse_config(struct config *config, struct string *http_response)
{
	struct list_head *p = NULL;

	if (!parser_parse_config(string_get(http_response), &config->peer_config)) {
		log_err("\nUnable to parse config\n");
		return 1;
	}

	list_for_each(p, &config->peer_config) {
		struct peer_config_list *i = container_of(p,
				struct peer_config_list, list);
		if (strcmp(i->peer_config->name, config->peerid) == 0) {
				config->my_peer = i->peer_config;
		}
	}

	if (config->my_peer == NULL) {
		log_err("Unable to find %s in config.\n", config->peerid);
		return 1;
	}

	return 0;
}

static void
main_free_parsed_info(struct config* config)
{
	parser_free_config(&config->peer_config);
}

static void
main_tempsave_fetched_config(struct config *config, struct string* cnf)
{
	int fd;
	static int NOTMPFILEWARNED = 0;

	if (str_is_empty(config->tmpconffile)) {
		if (NOTMPFILEWARNED) return;
		log_debug("Info: not tempsaving fetched config. Set $tmpconffile in chaosvpn.conf to enable.");
	        NOTMPFILEWARNED = 1;
		return;
	}

	fd = open(config->tmpconffile, O_WRONLY | O_CREAT, 0600);
	if (fd == -1) return;

	if (write(fd, string_get(cnf), string_length(cnf)) != string_length(cnf)) {
		(void)unlink(config->tmpconffile);
	}
	close(fd);
}

static int
main_load_previous_config(struct config *config, struct string* cnf)
{
	int fd;
	struct stat sb;
	intptr_t readbytes;
	int retval = 1;

	if (str_is_empty(config->tmpconffile)) return 1;

	fd = open(config->tmpconffile, O_RDONLY);
	if (fd == -1) return 1;

	if (fstat(fd, &sb)) return 1;
	if (string_read(cnf, fd, sb.st_size, &readbytes)) {
		log_err("Error: not enough memory to read stored config file.");
		string_clear(cnf);
		goto bail_out;
	}

	if (readbytes != sb.st_size) {
		log_err("Error: unable to fully read stored config file.");
		string_clear(cnf);
		goto bail_out;
	}

	retval = 0;
bail_out:
	close(fd);
	return retval;
}

static int
main_create_backup(struct config *config)
{
	int retval = 1;
	struct string base_backup_fn;

	if (string_init(&base_backup_fn, 512, 512)) return 1; /* don't goto bail_out here */
	if (string_concat(&base_backup_fn, config->base_path)) goto bail_out;
	if (string_concatb(&base_backup_fn, ".old", 5)) goto bail_out;

	retval = fs_cp_r(config->base_path, string_get(&base_backup_fn));

	/* fall through */
bail_out:
	string_free(&base_backup_fn);
	return retval;
}

static int
main_cleanup_hosts_subdir(struct config *config)
{
	int retval = 1;
	struct string hosts_dir;
	
	if (string_init(&hosts_dir, 512, 512)) return 1; /* don't goto bail_out here */
	if (string_concat(&hosts_dir, config->base_path)) goto bail_out;
	if (string_concat(&hosts_dir, "/hosts")) goto bail_out;

	retval = fs_empty_dir(string_get(&hosts_dir));

	/* fall through */
bail_out:
	string_free(&hosts_dir);
	return retval;
}

static void
main_unlink_pidfile(struct config *config)
{
	if (str_is_empty(config->pidfile))
		return;

	(void)unlink(config->pidfile);
}

static void
sigchild(int sig /*__unused*/)
{
	struct config *config = config_get();

	log_err("tincd terminated. Restarting in %d seconds.", config->tincd_restart_delay);
	if (!daemon_sigchld(&di_tincd, config->tincd_restart_delay)) {
		log_err("unable to restart tincd. Terminating.");
		exit(EXIT_FAILURE);
	}
}

static void
sigterm(int sig /*__unused*/)
{
	r_sigterm = 1;
}

static void
sigint(int sig /*__unused*/)
{
	r_sigint = 1;
}

static void
sigint_holdon(int sig /*__unused*/)
{
	log_info("I'm doing me best, please be patient for a little, will ya?");
}

