#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <dirent.h>

#include "string/string.h"
#include "fs.h"


static int fs_ensure_suffix(struct string*);
static int handledir(struct string*, struct string*);
static int fs_ensure_z(struct string* s);


/* Note the function definition -- this CANNOT take a const char for the path!
 * Returns 0 on success, -1 on error.  errno should be set.
 * On success, path is returned intact.  On failure, path is undefined.
 */
int fs_mkdir_p( char *path, mode_t mode ) {
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

int
fs_get_cwd(struct string* s)
{
	char stack[128];
	char* path;
	size_t bta;
	int retval;

	path = stack;
	if (getcwd(path, 128) != NULL) {
		if (string_concat(s, path)) return 1;
		if (string_get(s)[s->_u._s.length - 1] == '/') return 0;
		return string_putc(s, '/');
	}
	if (errno != ERANGE) return 1;
	for (bta = 4096; bta < 65536; bta+=4096) {
		path = malloc(bta);
		if (getcwd(path, bta) != NULL) {
			retval = string_concat(s, path);
			free(path);
			if (retval) return retval;
			if (string_get(s)[s->_u._s.length - 1] == '/') return 0;
			return string_putc(s, '/');
		}
		free(path);
		if (errno != ERANGE) return 1;
	}
	return 0;
}

int
fs_cp_file(const char const* src, const char const* dst)
{
	int fh_source;
	int fh_destination;
	struct stat stat_source;
	ssize_t filesize;
	ssize_t readbytes;
	ssize_t writtenbytes;
	char* buf;

	if (stat(src, &stat_source)) return 1;
	fh_source = open(src, O_RDONLY);
	if (fh_source == -1) return 1;
	fh_destination = open(dst, O_CREAT | O_WRONLY, stat_source.st_mode & 07777);
	if (fh_destination == -1) {
		close(fh_source);
		return 1;
	}
	filesize = stat_source.st_size;
	buf = malloc(stat_source.st_blksize);
	if (!buf) {
		close(fh_source);
		close(fh_destination);
		return 1;
	}
	while (filesize > 0) {
		readbytes = read(fh_source, buf, stat_source.st_blksize);
		writtenbytes = write(fh_destination, buf, readbytes);
		if (writtenbytes != readbytes) {
			close(fh_source);
			close(fh_destination);
			free(buf);
			return 1;
		}
		filesize -= writtenbytes;
	}
	close(fh_source);
	close(fh_destination);
	free(buf);
	return 0;
}

static int
handledir(struct string* src, struct string* dst)
{
	DIR* dir;
	struct dirent* dirent;
	size_t srcdirlen;
	size_t dstdirlen;
	struct stat st;
	struct timeval tv[2];
	int retval = 1;

	if (fs_ensure_z(src)) return 1;
	if (fs_ensure_z(dst)) return 1;

	if (stat(string_get(src), &st)) return 1;
	(void)mkdir(string_get(dst), st.st_mode & 07777);

	if (chdir(string_get(src))) return 1;
	dir = opendir(string_get(src));
	if (!dir) return 1;

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
		if (S_ISDIR(st.st_mode)) {
			srcdirlen = src->_u._s.length;
			dstdirlen = dst->_u._s.length;
			if (string_concat(src, dirent->d_name)) goto bail_out;
			if (fs_ensure_suffix(src)) goto bail_out;
			if (string_concat(dst, dirent->d_name)) goto bail_out;
			if (fs_ensure_suffix(dst)) goto bail_out;
			if (handledir(src, dst)) goto bail_out;
			if (utimes(string_get(dst), tv)) {
				(void)fprintf(stderr, "fs_cp_r: warning: utimes failed for %s\n", string_get(dst));
			}
			src->_u._s.length = srcdirlen;
			dst->_u._s.length = dstdirlen;
			(void)fs_ensure_z(src);
			(void)fs_ensure_z(dst);
			if (chdir(string_get(src))) goto bail_out;
		} else if (S_ISREG(st.st_mode)) {
			dstdirlen = dst->_u._s.length;
			if (string_concat(dst, dirent->d_name)) goto bail_out;
			if (fs_ensure_z(dst)) goto bail_out;
			if (fs_cp_file(dirent->d_name, string_get(dst))) goto bail_out;
			if (utimes(string_get(dst), tv)) {
				(void)fprintf(stderr, "fs_cp_r: warning: utimes failed for %s\n", string_get(dst));
			}
			dst->_u._s.length = dstdirlen;
		}
	}
	retval = 0;
	/* fallthrough */
bail_out:
	closedir(dir);
	return retval;
}

static int
fs_ensure_z(struct string* s)
{
	if (string_putc(s, 0)) return 1;
	--s->_u._s.length;
	return 0;
}

static int
fs_ensure_suffix(struct string* s)
{
	if (string_get(s)[s->_u._s.length - 1] == '/') return 0;
	if (string_putc(s, '/')) return 1;
	return fs_ensure_z(s);
}

int
fs_cp_r(char* src, char* dest)
{
	struct string source;
	struct string destination;
	struct string curwd;
	int retval = 1;

	(void)string_init(&source, 512, 512);
	(void)string_init(&destination, 512, 512);
	(void)string_init(&curwd, 512, 512);

	if (fs_get_cwd(&curwd)) goto nrcwd_bail_out;
	if (*src != '/') if (fs_get_cwd(&source)) goto bail_out;
	if (string_concat(&source, src)) goto bail_out;
	if (fs_ensure_suffix(&source)) goto bail_out;
	if (*dest != '/') if (fs_get_cwd(&destination)) goto bail_out;
	if (string_concat(&destination, dest)) goto bail_out;
	if (fs_ensure_suffix(&destination)) goto bail_out;


	retval = handledir(&source, &destination);

	/* fallthrough */
bail_out:
    if (fs_ensure_z(&curwd)) return 1;
	if(chdir(string_get(&curwd))) {
		(void)fprintf(stderr, "fs_cp_r: couldn't restore old cwd %s\n", string_get(&curwd));
	}
nrcwd_bail_out:
	string_free(&source);
	string_free(&destination);
    string_free(&curwd);
	return retval;
}

int
fs_writecontents(const char const* fn, const char const* cnt, const size_t len, const int mode)
{
	int fh;
	size_t bw;
	fh = open(fn, O_CREAT | O_WRONLY | O_TRUNC, mode);
	if (fh == -1) {
		return 1;
	}
	/* ABABAB: should throw proper error here */
	bw = write(fh, cnt, len);
	(void)close(fh);
	(void)chmod(fn, mode);
	return (len != bw);
}


int
fs_writecontents_safe(const char const* dir, const char const* fn, const char const* cnt, const int len, const int mode)
{
	char *buf = NULL, *ptr = NULL;
	int res;
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

