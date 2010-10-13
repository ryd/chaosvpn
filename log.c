
#include "log.h"

void
log_init(int *argc, char ***argv, int logopt, int logfac)
{
  char *progname;
  
  progname = (progname = strrchr((*argv)[0], '/')) ? progname+1 : (*argv)[0];
  openlog(progname, logopt, logfac);
}

