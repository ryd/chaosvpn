#ifndef __FS_H
#define __FS_H

#define NOERR   (0)

#include <sys/stat.h>
#include "string/string.h"

extern int fs_writecontents_safe(const char const* dir, const char const* fn, const char const* cnt, const int len, const int mode);
extern int fs_writecontents(const char const* fn, const char const* cnt, const size_t len, const int mode);
extern int fs_mkdir_p(char *, mode_t);
extern int fs_cp_r(char*, char*);
extern int fs_empty_dir(char*);
extern int fs_get_cwd(struct string*);
extern int fs_read_file(struct string *buffer, char *fname);
extern int fs_read_fd(struct string *buffer, int fd);

extern int fs_backticks_exec(const char *cmd, struct string *outputbuffer);

#endif
