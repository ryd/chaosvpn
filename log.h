
#ifndef __LOG__
#define __LOG__

#include <syslog.h>

#define log_emerg(fmt , args...)      (void)syslog(LOG_EMERG,  (fmt) , ## args)
#define log_alert(fmt , args...)      (void)syslog(LOG_ALERT,  (fmt) , ## args)
#define log_err(fmt , args...)        (void)syslog(LOG_ERR,    (fmt) , ## args)
#define log_warn(fmt , args...)       (void)syslog(LOG_WARNING,(fmt) , ## args)
#define log_crit(fmt , args...)       (void)syslog(LOG_ERR,    (fmt) , ## args)
#define log_note(fmt , args...)       (void)syslog(LOG_NOTICE, (fmt) , ## args)
#define log_info(fmt , args...)       (void)syslog(LOG_INFO,   (fmt) , ## args)
#define log_debug(fmt , args...)      (void)syslog(LOG_DEBUG,  (fmt) , ## args)

#define LOG_CRASH(fmt, args...) \
  do { \
    (void)syslog(LOG_ERR, "CRASH @ " __FILE__ ":%d " fmt, __LINE__, ## args); \
    closelog(); \
    exit(2); \
  } while (0)

#define LOG_ERREXIT(fmt, args...) \
  do { \
    (void)syslog(LOG_ERR, "ERREXIT: %s @ " __FILE__ ":%d (%s) " fmt, \
      strerror(errno), \
      __LINE__, \
      __PRETTY_FUNCTION__, ## args); \
    closelog(); \
    exit(2); \
  } while (0)


extern void log_init(int *argc, char ***argv, int logopt, int logfac);

#endif
