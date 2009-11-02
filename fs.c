#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

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
	return len == bw;
}
