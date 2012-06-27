#include <errno.h>
#include <signal.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#ifndef WIN32
#include <sys/wait.h>
#endif

#include "chaosvpn.h"

static void
fix_fds (void)
{
    /* Bad Things Happen if stdin, stdout, and stderr have been closed
       (as by the `sh incantation "attraction >&- 2>&-").  When you do
       that, the X connection gets allocated to one of these fds, and
       then some random library writes to stderr, and random bits get
       stuffed down the X pipe, causing "Xlib: sequence lost" errors.
       So, we cause the first three file descriptors to be open to
       /dev/null if they aren't open to something else already.  This
       must be done before any other files are opened (or the closing
       of that other file will again free up one of the "magic" first
       three FDs.)

       We do this by opening /dev/null three times, and then closing
       those fds, *unless* any of them got allocated as #0, #1, or #2,
       in which case we leave them open.  Gag.

       Really, this crap is technically required of *every* X program,
       if you want it to be robust in the face of "2>&-".
     */
    int fd0 = open ("/dev/null", O_RDWR);
    int fd1 = open ("/dev/null", O_RDWR);
    int fd2 = open ("/dev/null", O_RDWR);
    if (fd0 > 2) close (fd0);
    if (fd1 > 2) close (fd1);
    if (fd2 > 2) close (fd2);
}

bool
daemonize(void)
{
#ifndef WIN32
    pid_t pid, sid;

    /* Fork off the parent process */
    pid = fork();
    if (pid < 0) {
        return false;
    }
    
    /* If we got a good PID, then we are the parent and can exit */
    if (pid > 0) {
        exit(EXIT_SUCCESS);
    }

    /* now in forked child */

    /* Fork again, to become a real daemon */
    pid = fork();
    if (pid < 0) {
        exit(1);
    } else if (pid > 0) {
        exit(EXIT_SUCCESS);
    }
    
    /* Change the file mode mask */
    umask(0);
    
    /* Create a new SID for the child process */
    sid = setsid();
    if (sid < 0) {
        /* we should log something here... */
        return false;
    }

    /* Change the current working directory */
    (void)chdir("/"); /* ignore errors */

    /* Close out the standard file descriptors */
    close(STDIN_FILENO);
    close(STDOUT_FILENO),
    close(STDERR_FILENO);
    
    /* Replace standard file descriptors with /dev/null */
    fix_fds();
#endif
    
    return true;
}

bool
daemon_init(struct daemon_info* di, const char* path, ...)
{
    int i;
    int numargs;
    va_list ap;

    di->di_stderr_fd[0] = -1;
    di->di_stderr_fd[1] = -1;
    di->di_stderr = NULL;
    di->di_pid = -1;
    di->di_path = strdup(path);

    va_start(ap, path);
    for (numargs = 0; va_arg(ap, char*) != NULL; ++numargs);
    va_end(ap);

    /* append one argument: NULL */
    ++numargs;
    di->di_arguments = (char**)malloc(sizeof(char*) * numargs);
    if (di->di_arguments == NULL) {
        goto bail_out;
    }
    memset(di->di_arguments, 0, sizeof(char*) * numargs);
    di->di_numarguments = numargs;
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

    return true;

bail_out:
    if (di->di_path != NULL) {
        free(di->di_path);
        di->di_path = NULL;
    }
    if (di->di_arguments != NULL) {
        for(i = 0; di->di_arguments[i] != NULL; i++) {
            free(di->di_arguments[i]);
        }
        free(di->di_arguments);
        di->di_arguments = NULL;
    }
    /* XXX: for now */
    free(di->di_envp);
    di->di_envp = NULL;
    return false;
}

bool
daemon_addparam(struct daemon_info* di, const char* param)
{
    int numargs;
    char** tmp;
    char* dparam;

    numargs = di->di_numarguments + 1;
    tmp = (char**)realloc(di->di_arguments, sizeof(char*) * numargs);
    if (tmp == NULL) return false;
    dparam = strdup(param);
    if (dparam == NULL) return false;
    di->di_arguments = tmp;
    ++di->di_numarguments;
    di->di_arguments[numargs - 2] = dparam;
    di->di_arguments[numargs - 1] = NULL;
    return true;
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
    di->di_arguments = NULL;
    
    /* XXX: for now */
    free(di->di_envp);
    di->di_envp = NULL;
    
    di->di_pid = -1;
    
    if (di->di_stderr_fd[0] != -1)
        close(di->di_stderr_fd[0]);
    if (di->di_stderr_fd[1] != -1)
        close(di->di_stderr_fd[1]);
    di->di_stderr_fd[0] = -1;
    di->di_stderr_fd[1] = -1;
    if (di->di_stderr)
        fclose(di->di_stderr);
    di->di_stderr = NULL;
}

bool
daemon_start(struct daemon_info* di)
{
#ifndef WIN32
    pid_t pid;

    if (di->di_stderr_fd[0] != -1)
        close(di->di_stderr_fd[0]);
    if (di->di_stderr_fd[1] != -1)
        close(di->di_stderr_fd[1]);
    if (pipe(di->di_stderr_fd) == -1) {
        log_err("daemon_start(): creating stderr pipe failed: %s", strerror(errno));
        return false;
    }

    fcntl(di->di_stderr_fd[0], O_NONBLOCK);
    fcntl(di->di_stderr_fd[1], O_NONBLOCK);

    di->di_stderr = fdopen(di->di_stderr_fd[0], "r");
    
    switch(pid = fork()) {
    case 0:
        (void)setsid();
        dup2(di->di_stderr_fd[1], STDERR_FILENO);
        close(di->di_stderr_fd[0]);
        close(di->di_stderr_fd[1]);
        (void)execve(di->di_path, di->di_arguments, di->di_envp);
        exit(EXIT_FAILURE);
    case -1:
        return false;
    default:
        close(di->di_stderr_fd[1]);
        di->di_stderr_fd[1] = -1;
        di->di_pid = pid;
        return true;
    }
#endif
    return true;
}

void
daemon_stop(struct daemon_info* di, const unsigned int sleepdelay)
{
#ifndef WIN32
    pid_t pgid;

    pgid = di->di_pid; //__getpgid(di->di_pid);
    if (pgid == -1) {
        return;
    }
    if (kill(pgid, SIGTERM)) {
        if (errno != ESRCH) {
            return;
        }
    }
    if (sleepdelay == 0) {
        /* no sigkill at all */
        return;
    }
    (void)sleep(sleepdelay);
    (void)kill(pgid, SIGKILL);
#endif
    return;
}

bool
daemon_sigchld(struct daemon_info* di, unsigned int waitbeforerestart)
{
#ifndef WIN32
    int status;

    waitpid(di->di_pid, &status, 0);
    if (waitbeforerestart != 0) {
        (void)sleep(waitbeforerestart);
    }
#endif
    return daemon_start(di);
}

