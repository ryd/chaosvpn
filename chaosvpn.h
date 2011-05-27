#ifndef __CHAOSVPN_H
#define __CHAOSVPN_H

#include <stdbool.h>
#include <time.h>
#include <openssl/evp.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <syslog.h>

#include "list.h"
#include "string/string.h"
#include "httplib/httplib.h"
#include "addrmask.h"
#include "strnatcmp.h"
#include "version.h"

#define TINC_DEFAULT_PORT "665"
/*
   ^^ Note: This 665 is a typo, it should have been 655 instead.
            But fixing this may mean creating imcompatibiliies between
            older versions of this program and current versions, peers
            using the default port would have to change their firewall
            rules - so just keep it.
            (The ancient perl version correctly used 655)
 */

#define TINC_DEFAULT_CIPHER "blowfish"
#define TINC_DEFAULT_COMPRESSION "0"
#define TINC_DEFAULT_DIGEST "sha1"

#define TUN_DEV   "/dev/net/tun"
#define TUN_PATH  "/dev/net"



#define NOERR   (0)

extern bool ar_is_ar_file(struct string *archive);
extern bool ar_extract(struct string *archive, char *membername, struct string *result);



struct string_list {
    struct list_head list;
    char *text;
};

typedef enum E_settings_list_entry_type {
	LIST_STRING,
	LIST_INTEGER,
	LIST_LIST /* reserved */
} settings_list_entry_type;

enum {
	HANDLER_START_TINCD=0,
	HANDLER_RESTART_TINCD=1,
	HANDLER_STOP=2,
	HANDLER_SIGNAL_OLD_TINCD=3
};


struct settings_list_entry {
	settings_list_entry_type etype;
	union {
		char* s;
		int i;
	} evalue;
};

struct settings_list {
	struct list_head list;
	struct settings_list_entry* e;
};


struct peer_config {
	char *name;
	char *gatewayhost;
	char *owner;
	char *use_tcp_only;
	struct list_head network;
	struct list_head network6;
	struct list_head route_network;
	struct list_head route_network6;
	char *hidden;
	char *silent;
	char *port;
	char *indirectdata;
	char *key;
	char *cipher;
	char *compression;
	char *digest;
	char *primary;
};

struct peer_config_list {
	struct list_head list;
	struct peer_config *peer_config;
};

struct config {
	char *peerid;
	char *vpn_ip;
	char *vpn_ip6;
	char *networkname;
	char *my_ip;
	char *tincd_bin;
	char *tincd_version;
	char *tincctl_bin;
	unsigned int tincd_debuglevel;
	unsigned int tincd_restart_delay;
	char *routemetric;
	char *routeadd;
	char *routeadd6;
	char *routedel;
	char *routedel6;
	char *postup;
	char *ifconfig;
	char *ifconfig6;
	char *master_url;
	char *base_path;
	char *pidfile;
	char *cookiefile;
	char *masterdata_signkey;
	char *tincd_graphdumpfile;
	char *tmpconffile;
	char *tincd_device;
	char *tincd_interface;
	char *tincd_user;
	struct string privkey;
	struct settings_list *exclude;
	struct peer_config *my_peer;
	struct list_head peer_config;
	time_t ifmodifiedsince;
	unsigned int update_interval;
	bool use_dynamic_routes;
	bool connect_only_to_primary_nodes;
	bool run_ifdown;
	struct settings_list *mergeroutes_supernet_raw;
	struct addr_info *mergeroutes_supernet;
	struct settings_list *ignore_subnets_raw;
	struct addr_info *ignore_subnets;
	struct settings_list *whitelist_subnets_raw;
	struct addr_info *whitelist_subnets;

	/* vars only used in configfile, dummy for c code: */
	char *password;
	char *vpn_netmask;

	/* commandline parameter: */
	char *configfile;
	bool daemonmode;
	bool oneshot;

	/* used to set various file permissions */
	uid_t tincd_uid;
	gid_t tincd_gid;
};

extern struct config* config_alloc(void);
extern bool config_init(struct config *config);
extern void config_free(struct config *config);

/* get pointer to already allocated and initialized config structure */
extern struct config* config_get(void);



extern void crypto_init(void);
extern void crypto_finish(void);

extern EVP_PKEY *crypto_load_key(const char *key, const bool is_private);
extern bool crypto_rsa_verify_signature(struct string *databuffer, struct string *signature, const char *pubkey);
extern bool crypto_rsa_decrypt(struct string *ciphertext, const char *privkey, struct string *decrypted);
extern bool crypto_aes_decrypt(struct string *ciphertext, struct string *aes_key, struct string *aes_iv, struct string *decrypted);
extern void crypto_warn_openssl_version_changed(void);



struct daemon_info {
    char* di_path;
    int di_numarguments;
    char** di_arguments;
    char** di_envp;

    int di_stderr_fd[2];
    FILE *di_stderr;

    pid_t di_pid;
};

extern bool daemonize(void);
extern bool daemon_init(struct daemon_info* di, const char* path, ...);
extern bool daemon_addparam(struct daemon_info* di, const char* param);
extern void daemon_free(struct daemon_info*);
extern bool daemon_start(struct daemon_info*);
extern void daemon_stop(struct daemon_info*, const unsigned int sleepdelay);
extern bool daemon_sigchld(struct daemon_info*, unsigned int waitbeforerestart);



extern int fs_writecontents_safe(const char*, const char*, const char*, const int, const int);
extern int fs_writecontents(const char * fn, const char * cnt, const size_t len, const int mode);
extern int fs_mkdir_p(char *, mode_t);
extern int fs_cp_r(char*, char*);
extern int fs_empty_dir(char*);
extern int fs_get_cwd(struct string*);
extern int fs_read_file(struct string *buffer, char *fname);
extern int fs_read_fd(struct string *buffer, int fd);

extern int fs_backticks_exec(const char *cmd, struct string *outputbuffer);




#define log_emerg(fmt , args...)      log_raw(LOG_EMERG,  (fmt) , ## args)
#define log_alert(fmt , args...)      log_raw(LOG_ALERT,  (fmt) , ## args)
#define log_err(fmt , args...)        log_raw(LOG_ERR,    (fmt) , ## args)
#define log_warn(fmt , args...)       log_raw(LOG_WARNING,(fmt) , ## args)
#define log_crit(fmt , args...)       log_raw(LOG_ERR,    (fmt) , ## args)
#define log_note(fmt , args...)       log_raw(LOG_NOTICE, (fmt) , ## args)
#define log_info(fmt , args...)       log_raw(LOG_INFO,   (fmt) , ## args)
#define log_debug(fmt , args...)      log_raw(LOG_DEBUG,  (fmt) , ## args)

#define LOG_CRASH(fmt, args...) \
  do { \
    log_raw(LOG_ERR, "CRASH @ " __FILE__ ":%d " fmt, __LINE__, ## args); \
    closelog(); \
    exit(2); \
  } while (0)

#define LOG_ERREXIT(fmt, args...) \
  do { \
    log_raw(LOG_ERR, "ERREXIT: %s @ " __FILE__ ":%d (%s) " fmt, \
      strerror(errno), \
      __LINE__, \
      __PRETTY_FUNCTION__, ## args); \
    closelog(); \
    exit(2); \
  } while (0)


extern void log_init(int *argc, char ***argv, int logopt, int logfac);
extern void log_raw(int priority, const char *format, ...);


extern bool parser_parse_config (char *data, struct list_head *config_list);
extern void parser_free_config(struct list_head* configlist);


extern bool pidfile_create_pidfile(const char *filename);


extern bool tinc_write_config(struct config*);
extern bool tinc_write_hosts(struct config *config);
extern bool tinc_write_updown(struct config*, bool up);
extern bool tinc_write_subnetupdown(struct config*, bool up);
extern char *tinc_get_version(struct config *config);
extern pid_t tinc_get_pid(struct config *config);
extern bool tinc_invoke_ifdown(struct config* config);


extern bool tun_check_or_create(); 


extern bool uncompress_inflate(struct string *compressed, struct string *uncompressed);


#endif
