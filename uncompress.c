#include <stdbool.h>
#include <stdio.h>
#include <zlib.h>

#include "chaosvpn.h"

bool
uncompress_inflate(struct string *compressed, struct string *uncompressed)
{
    int retval;
    z_stream strm;
    unsigned char outbuf[1024];
    int have;

    /* allocate inflate state */
    memset(&strm, 0, sizeof(strm)); /* paranoia */
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    strm.avail_in = 0;
    strm.next_in = Z_NULL;
    if (inflateInit(&strm) != Z_OK)
        return false;

    strm.avail_in = string_length(compressed);
    strm.next_in = (unsigned char*)string_get(compressed);
    
    do {
        strm.avail_out = sizeof(outbuf);
        strm.next_out = outbuf;
        retval = inflate(&strm, Z_FINISH);
        switch (retval) {
            case Z_STREAM_ERROR:
                /* state clobbered */
                return false;
                break;
            case Z_NEED_DICT:
                retval = Z_DATA_ERROR; /* and fall through */
            case Z_DATA_ERROR:
            case Z_MEM_ERROR:
                (void)inflateEnd(&strm);
                return false;
        }
        
        have = sizeof(outbuf) - strm.avail_out;
        string_concatb(uncompressed, (char *)outbuf, have);
    } while (strm.avail_out == 0);

    (void)inflateEnd(&strm);

    if (retval != Z_STREAM_END) {
        fprintf(stderr, "decompression failed\n");
        return false;
    }
    
    return true;
}
