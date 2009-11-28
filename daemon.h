#ifndef __DAEMON_H
#define __DAEMON_H

#include <sys/types.h>

struct daemon_info {
    char* di_path;
    char** di_arguments;
    char** di_envp;

    pid_t di_pid;
};

extern int daemon_init(struct daemon_info*, char*, ...);
extern void daemon_free(struct daemon_info*);
extern int daemon_start(struct daemon_info*);
extern int daemon_stop(struct daemon_info*, unsigned int);
extern int daemon_sigchld(struct daemon_info*, unsigned int);

#endif
