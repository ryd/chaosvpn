#ifndef __FS_H
#define __FS_H

extern bool	fs_writecontents_safe(const char const* dir, const char const* fn, const char const* cnt, const int len, const int mode);
extern bool	fs_writecontents(const char const* fn, const char const* cnt, const int len, const int mode);

#endif
