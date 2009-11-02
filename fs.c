#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

/* Note the function definition -- this CANNOT take a const char for the path!
 * Returns 0 on success, -1 on error.  errno should be set.
 * On success, path is returned intact.  On failure, path is undefined.
 */
int fs_mkdir_p( char *path, mode_t mode ) {
	int err,pos,i;

	err = mkdir( path, mode );
	if( err && errno == ENOENT ) {

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
		if( err )
			return err;

		path[ pos ] = '/';
		err = mkdir( path, mode );
	}

	return err;
}


int fs_cp_r(char *src, char *dest) {
	//spaceholder
	return 0;
}

int
fs_writecontents(const char const* fn, const char const* cnt, const int len, const int mode)
{
	int fh;
	int bw;
	fh = open(fn, O_CREAT | O_WRONLY, mode);
	bw = write(fh, cnt, len);
	close(fh);
	return len != bw;
}


int
fs_writecontents_safe(const char const* dir, const char const* fn, const char const* cnt, const int len, const int mode)
{
    char *buf, *ptr;
    int res;
    unsigned int dlen, reqlen;

    dlen = strlen(dir) + 1;
    if(!dlen) return 1;
    reqlen = dlen + strlen(fn) + 1;
    if(reqlen < dlen) return 1;
    if(!(buf = ptr = malloc(reqlen))) return 1;
    strcpy(ptr, dir);
    *(ptr + dlen - 1) = '/';
    *(ptr + dlen) = 0;
    strcat(ptr, fn);
    for(ptr=buf+dlen;*ptr;ptr++) if(*ptr== '/') *ptr='_';
    res = fs_writecontents(buf, cnt, len, mode);
    free(buf);
    return res;
}

