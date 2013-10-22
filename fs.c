#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <dirent.h>

#include "chaosvpn.h"


#define NOERR (0)


/* Note the function definition -- this CANNOT take a const char for the path!
 * Returns 0 on success, -1 on error.  errno should be set.
 * On success, path is returned intact.  On failure, path is undefined.
 */
int fs_mkdir_p( char *path, mode_t mode )
{
	int err,pos,i;

	err = mkdir( path, mode );
	if( (err != NOERR) && errno == ENOENT ) {

		/* find the last occurance of '/' */
		for( i = 0, pos = 0; path[ i ] != '\0'; i++ ) {
			if( path[ i ] == '/' )
				pos = i;
		}

		/* return -1 if '/' is not found, or found only at position 1
		 * ("/" is not a valid mkdir) */
		if( pos == 0 )
			return -1;

		path[ pos ] = '\0';
		err = fs_mkdir_p( path, mode );
		if( err != NOERR )
			return err;

		path[ pos ] = '/';
		err = mkdir( path, mode );
	}

    return err;
}

static bool
fs_ensure_suffix(struct string* s)
{
	if (string_get(s)[s->_u._s.length - 1] == '/') return true;
	if (!string_putc(s, '/')) return false;
	return string_ensurez(s);
}

bool
fs_get_cwd(struct string* s)
{
	char stackpath[256];
	char* allocpath;
	size_t bta;
	int retval;

	if (getcwd(stackpath, sizeof(stackpath)) != NULL) {
		if (!string_concat(s, stackpath)) return false;
		if (string_get(s)[s->_u._s.length - 1] == '/') return true;
		return string_putc(s, '/');
	}
	if (errno != ERANGE) return false;
	for (bta = 4096; bta < 65536; bta+=4096) {
		allocpath = malloc(bta);
		if (allocpath == NULL) { return false; }
		if (getcwd(allocpath, bta) != NULL) {
			retval = string_concat(s, allocpath);
			free(allocpath);
			if (!retval) return false;
			if (string_get(s)[s->_u._s.length - 1] == '/') return true;
			return string_putc(s, '/');
		}
		free(allocpath);
		if (errno != ERANGE) return false;
	}
	return true;
}

static bool
fs_cp_file(const char *src, const char *dst)
{
	int fh_source;
	int fh_destination;
	struct stat stat_source;
	ssize_t filesize;
	ssize_t readbytes;
	ssize_t writtenbytes;
	char buf[65536];
	bool retval = false;

	if (stat(src, &stat_source)) return false;
	fh_source = open(src, O_RDONLY);
	if (fh_source == -1) return false;
	fh_destination = open(dst, O_CREAT | O_WRONLY, stat_source.st_mode & 07777);
	if (fh_destination == -1) {
		goto bail_out_close_source;
	}
	filesize = stat_source.st_size;
	while (filesize > 0) {
		readbytes = read(fh_source, &buf, sizeof(buf));
		writtenbytes = write(fh_destination, &buf, readbytes);
		if (writtenbytes != readbytes) {
			goto bail_out_close_dest;
		}
		filesize -= writtenbytes;
	}

	retval = true;

bail_out_close_dest:
	(void)close(fh_destination);
bail_out_close_source:
	(void)close(fh_source);

	return retval;
}

static bool
handledir(struct string* src, struct string* dst)
{
	DIR* dir;
	struct dirent* dirent;
	size_t srcdirlen;
	size_t dstdirlen;
	struct stat st;
	struct timeval tv[2];
	bool retval = false;

	if (!string_ensurez(src)) return false;
	if (!string_ensurez(dst)) return false;

	//log_debug("fs_cp_r: handledir(%s, %s)", string_get(src), string_get(dst));

	if (stat(string_get(src), &st)) {
		log_warn("fs_cp_r: stat(%s) failed: %s", string_get(src), strerror(errno));
		return false;
	}
	(void)mkdir(string_get(dst), st.st_mode & 07777);

	if (chdir(string_get(src))) {
		log_warn("fs_cp_r: chdir(%s) failed: %s", string_get(src), strerror(errno));
		return false;
	}
	dir = opendir(string_get(src));
	if (!dir) {
		log_warn("fs_cp_r: opendir(%s) failed: %s", string_get(src), strerror(errno));
		return false;
	}

	tv[0].tv_usec = 0;
	tv[1].tv_usec = 0;

	while ((dirent = readdir(dir))) {
		if (stat(dirent->d_name, &st)) continue;
		if ((*dirent->d_name == '.') &&
			((dirent->d_name[1] == 0) ||
				((dirent->d_name[1] == '.') &&
				(dirent->d_name[2] == 0)))) continue;
		tv[0].tv_sec = st.st_atime;
		tv[1].tv_sec = st.st_mtime;

		//log_debug("handle %s %s", string_get(src), dirent->d_name);

		if (S_ISDIR(st.st_mode)) {
			srcdirlen = src->_u._s.length;
			dstdirlen = dst->_u._s.length;
			if (!string_concat(src, dirent->d_name)) goto bail_out;
			if (!fs_ensure_suffix(src)) goto bail_out;
			if (!string_concat(dst, dirent->d_name)) goto bail_out;
			if (!fs_ensure_suffix(dst)) goto bail_out;
			if (!handledir(src, dst)) goto bail_out;
#ifndef WIN32
			if (utimes(string_get(dst), tv)) {
				log_warn("fs_cp_r: warning: utimes failed for %s", string_get(dst));
			}
#endif
			src->_u._s.length = srcdirlen;
			dst->_u._s.length = dstdirlen;
			(void)string_ensurez(src);
			(void)string_ensurez(dst);
			if (chdir(string_get(src))) {
				log_warn("fs_cp_r: chdir(%s) failed: %s", string_get(src), strerror(errno));
				goto bail_out;
			}
		} else if (S_ISREG(st.st_mode)) {
			dstdirlen = dst->_u._s.length;
			if (!string_concat(dst, dirent->d_name)) goto bail_out;
			if (!string_ensurez(dst)) goto bail_out;
			if (!fs_cp_file(dirent->d_name, string_get(dst))) {
				log_warn("fs_cp_r: copy %s to %s failed: %s", dirent->d_name, string_get(dst), strerror(errno));
				goto bail_out;
			}
#ifndef WIN32
			if (utimes(string_get(dst), tv)) {
				log_warn("fs_cp_r: warning: utimes failed for %s", string_get(dst));
			}
#endif
			dst->_u._s.length = dstdirlen;
		}
	}

	retval = true;

	/* fallthrough */

bail_out:
	(void)closedir(dir);
	return retval;
}

bool
fs_cp_r(char* src, char* dest)
{
	struct string source;
	struct string destination;
	struct string curwd;
	bool retval = false;

	(void)string_init(&source, 512, 512);
	(void)string_init(&destination, 512, 512);
	(void)string_init(&curwd, 512, 512);

	if (!fs_get_cwd(&curwd)) goto nrcwd_bail_out;
	if (*src != '/') if (!fs_get_cwd(&source)) goto bail_out;
	if (!string_concat(&source, src)) goto bail_out;
	if (!fs_ensure_suffix(&source)) goto bail_out;
	if (*dest != '/') if (!fs_get_cwd(&destination)) goto bail_out;
	if (!string_concat(&destination, dest)) goto bail_out;
	if (!fs_ensure_suffix(&destination)) goto bail_out;


	retval = handledir(&source, &destination);

	/* fallthrough */
bail_out:
	if (!string_ensurez(&curwd)) goto nrcwd_bail_out;
	if (chdir(string_get(&curwd))) {
		log_err("fs_cp_r: couldn't restore old cwd %s\n", string_get(&curwd));
	}

nrcwd_bail_out:
	string_free(&source);
	string_free(&destination);
	string_free(&curwd);
	return retval;
}

/* unlinks all regular files in a specified directory */
/* does not use recursion! does not touch symlinks! */
bool
fs_empty_dir(char* dest)
{
	struct string dst;
	size_t dstdirlen;
	struct string curwd;
	DIR* dir;
	struct dirent* dirent;
	struct stat st;
	bool retval = false;

	(void)string_init(&dst, 512, 512);
	(void)string_init(&curwd, 512, 512);

	if (!fs_get_cwd(&curwd)) goto nrcwd_bail_out;
	if (*dest != '/') if (!fs_get_cwd(&dst)) goto bail_out;
	if (!string_concat(&dst, dest)) goto bail_out;

	if (!string_ensurez(&dst)) goto bail_out;

	if (stat(string_get(&dst), &st)) {
		retval = true; /* non-existance is not an error reason! */
		goto bail_out;
	}
	if (!S_ISDIR(st.st_mode)) goto bail_out;

	if (!fs_ensure_suffix(&dst)) goto bail_out;
	if (chdir(string_get(&dst))) goto bail_out;

	dir = opendir(string_get(&dst));
	if (!dir) goto bail_out;

	while ((dirent = readdir(dir))) {
		if (stat(dirent->d_name, &st)) continue;
		if ((*dirent->d_name == '.') &&
			((dirent->d_name[1] == 0) ||
				((dirent->d_name[1] == '.') &&
				(dirent->d_name[2] == 0)))) continue;

		if (S_ISREG(st.st_mode)) {
			dstdirlen = dst._u._s.length;
			if (!string_concat(&dst, dirent->d_name)) goto bail_out_closedir;
			if (unlink(string_get(&dst))) {
				log_err("fs_empty_dir: failed to unlink %s: %s\n", string_get(&dst), strerror(errno));
			}
			dst._u._s.length = dstdirlen;
		}
	}

	retval = true;

	/* fallthrough */
bail_out_closedir:
	(void)closedir(dir);

bail_out:
	if (!string_ensurez(&curwd)) goto nrcwd_bail_out;
	if (chdir(string_get(&curwd))) {
		log_err("fs_empty_dir: couldn't restore old cwd %s\n", string_get(&curwd));
	}

nrcwd_bail_out:
	string_free(&dst);
	string_free(&curwd);
	return retval;
}

bool
fs_writecontents(const char *fn,
                 const char *cnt,
                 const size_t len,
                 const int mode)
{
	int fh;
	size_t bw;
	fh = open(fn, O_CREAT | O_WRONLY | O_TRUNC, mode);
	if (fh == -1) {
		return false;
	}
	/* ABABAB: should throw proper error here */
	bw = write(fh, cnt, len);
	(void)close(fh);
	if (len != bw) return false;
	return true;
}


bool
fs_writecontents_safe(const char *dir,
                      const char *fn,
                      const char *cnt,
                      const int len,
                      const int mode)
{
	char *buf = NULL, *ptr = NULL;
	bool res;
	unsigned int dlen, reqlen;

	dlen = strlen(dir) + 1;
	/* ABABAB: code never executed: if(!dlen) return 1; */
	reqlen = dlen + strlen(fn) + 1;	/* Gives us two extra bytes at end when one is required */
	/* ABABAB: code never executed: if(reqlen < dlen) return 1; */
	if (NULL == (ptr = malloc((size_t)reqlen))) return 1;
	buf = ptr;			/* for the write below */
	strcpy(ptr, dir);
	*(ptr + dlen - 1) = '/';
	*(ptr + dlen) = '\0';
	strcat(ptr, fn);
	for(ptr=buf+dlen;'\0' != *ptr;ptr++) if('/' == *ptr) *ptr='_';
	res = fs_writecontents(buf, cnt, len, mode);
	free(buf);
	return res;
}

/* read file into struct string */
/* note: not NULL terminated! binary clean */
bool
fs_read_file(struct string *buffer, char *fname)
{
	bool retval = false;
	FILE *fp;
	char chunk[4096];
	size_t read_size;

	fp = fopen(fname, "r");
	if (fp==NULL) return false;

	while ((read_size = fread(&chunk, 1, sizeof(chunk), fp)) > 0) {
		if (!string_concatb(buffer, chunk, read_size)) goto bail_out;
	}

	if (!ferror(fp))
		retval = true;

bail_out:
	fclose(fp);
	return retval;
}

/* read everything from fd into struct string */
/* note: not NULL terminated! binary clean */
bool
fs_read_fd(struct string *buffer, FILE *fd)
{
	bool retval = false;
	char chunk[4096];
	ssize_t read_size;
	
	while ((read_size = fread(&chunk, 1, sizeof(chunk), fd)) > 0) {
		if (!string_concatb(buffer, chunk, read_size)) goto bail_out;
	}
	
	retval = true;

bail_out:
	return retval;
}

/* execute cmd, return stdout output in struct string *outputbuffer */
bool
fs_backticks_exec(const char *cmd, struct string *outputbuffer)
{
	bool retval = false;
	FILE *pfd;

	pfd = popen(cmd, "r");

	if (!pfd) {
		log_err("fs_backticks_exec: pipe() failed\n");
		goto bail_out;
	}
	
	retval = fs_read_fd(outputbuffer, pfd);
	fclose(pfd);
	
bail_out:
	return retval;
}
