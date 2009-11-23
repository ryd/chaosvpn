#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <fts.h>

#include "string/string.h"
#include "fs.h"

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

static int
fs_fts_compare(const FTSENT** a, const FTSENT** b)
{
    int fts_info_a;
    int fts_info_b;

    fts_info_a = (*a)->fts_info;
    if (fts_info_a == FTS_ERR) return 0;
    if (fts_info_a == FTS_NS) return 0;
    if (fts_info_a == FTS_DNR) return 0;
    fts_info_b = (*b)->fts_info;
    if (fts_info_b == FTS_ERR) return 0;
    if (fts_info_b == FTS_NS) return 0;
    if (fts_info_b == FTS_DNR) return 0;
    if (fts_info_a == FTS_D) return -1;
    if (fts_info_b == FTS_D) return 1;
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
            return 1;
        }
        filesize -= writtenbytes;
    }
    close(fh_source);
    close(fh_destination);
    return 0;
}


int
fs_cp_r(char* src, char* dest)
{
    FTS* fts;
    FTSENT* entry;
    char* srces[2];
    struct stat sb;

    char* srcpath;
    struct string dstpath;
    int splen;
    int dplen;
    int dpslash;

    if (stat(src, &sb)) return 1;
    /* This routine only accepts directories as source */
    if (!S_ISDIR(sb.st_mode)) return 1;

    srces[0] = src;
    srces[1] = NULL;
    fts = fts_open(srces, FTS_XDEV | FTS_NOCHDIR, fs_fts_compare);
    if (!fts) return 1;

    splen = strlen(src);
    if (src[splen - 1] != '/') ++splen;
    dplen = strlen(dest);
    dpslash = (dest[dplen - 1] == '/');

    string_init(&dstpath, 512, 512);
    while((entry = fts_read(fts))) {
        srcpath = entry->fts_path + splen;
        string_clear(&dstpath);
        string_concatb(&dstpath, dest, dplen);
        if (!dpslash) string_concatb(&dstpath, "/", 1);
        string_concat(&dstpath, srcpath);
        if (entry->fts_info & FTS_D) {
            mkdir(string_get(&dstpath), entry->fts_statp->st_mode & 07777);
        }
        if (entry->fts_info & FTS_F) {
            if (fs_cp_file(entry->fts_path, string_get(&dstpath))) return 1;
        }
    }
    fts_close(fts);
    string_free(&dstpath);

    return 0;
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

