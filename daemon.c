#include <error.h>
#include <errno.h>
#include <signal.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#include "daemon.h"

int
daemon_init(struct daemon_info* di, char* path, ...)
{
    int i;
    int numargs;
    va_list ap;

    di->di_path = strdup(path);

    va_start(ap, path);
    for (numargs = 0; va_arg(ap, char*) != NULL; ++numargs);
    va_end(ap);

    di->di_arguments = (char**)malloc(sizeof(char*) * (numargs + 1));
    if (di->di_arguments == NULL) {
        goto bail_out;
    }
    va_start(ap, path);
    for (i = 0; i < numargs; i++) {
        di->di_arguments[i] = va_arg(ap, char*);
        if (di->di_arguments[i] != NULL) {
            di->di_arguments[i] = strdup(di->di_arguments[i]);
            if (di->di_arguments[i] == NULL) {
                goto bail_out;
            }
        }
    }
    va_end(ap);

    // XXX: for now
    di->di_envp = (char**)malloc(sizeof(char*));
    if (di->di_envp == NULL) {
        goto bail_out;
    }
    di->di_envp[0] = NULL;

    di->di_pid = -1;
    return 0;

bail_out:
    if (di->di_path != NULL) {
        free(di->di_path);
    }
    if (di->di_arguments != NULL) {
        for(i = 0; di->di_arguments[i] != NULL; i++) {
            free(di->di_arguments[i]);
        }
        free(di->di_arguments);
    }
    /* XXX: for now */
    free(di->di_envp);
    return 1;
}

void
daemon_free(struct daemon_info* di)
{
    int i;
    free(di->di_path);
    for (i = 0; di->di_arguments[i] != NULL; i++) {
        free(di->di_arguments[i]);
    }
    free(di->di_arguments);
    /* XXX: for now */
    free(di->di_envp);
    di->di_pid = -1;
}

int
daemon_start(struct daemon_info* di)
{
    pid_t pid;
    // exect(di->di_path, di->di_arguments, di->di_envp);
    switch(pid = fork()) {
    case 0:
        setsid();
        (void)execv(di->di_path, di->di_arguments);
    case -1:
        return -1;
    default:
        di->di_pid = pid;
        return 0;
    }
}

int
daemon_stop(struct daemon_info* di, unsigned int sleepdelay)
{
    pid_t pgid;

    pgid = __getpgid(di->di_pid);
    if (pgid == -1) {
        return 0;
    }
    if (kill(pgid, SIGTERM)) {
        if (errno != ESRCH) {
            return 1;
        }
    }
    if (sleepdelay == 0) {
        /* no sigkill at all */
        return 0;
    }
    sleep(sleepdelay);
    (void)kill(pgid, SIGKILL);
    return 0;
}

int
daemon_sigchld(struct daemon_info* di, unsigned int waitbeforerestart)
{
    pid_t pid;
    int status;

    pid = waitpid(di->di_pid, &status, 0);
    if (waitbeforerestart != 0) {
        sleep(waitbeforerestart);
    }
    return daemon_start(di);
}

