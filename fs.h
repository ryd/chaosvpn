#ifndef __FS_H
#define __FS_H

extern int	fs_writecontents_safe(const char const* dir, const char const* fn, const char const* cnt, const int len, const int mode);
extern int	fs_writecontents(const char const* fn, const char const* cnt, const int len, const int mode);
extern int fs_mkdir_p(char *, mode_t);

#endif
