#include <stdio.h>
#include <zlib.h>

#include "chaosvpn.h"

int
uncompress_inflate(struct string *compressed, struct string *uncompressed)
{
    int retval;
    z_stream strm;
    unsigned char outbuf[1024];
    int have;

    /* allocate inflate state */
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    strm.avail_in = 0;
    strm.next_in = Z_NULL;
    if (inflateInit(&strm) != Z_OK)
        return 1;

    strm.avail_in = string_length(compressed);
    strm.next_in = (unsigned char*)string_get(compressed);
    
    do {
        strm.avail_out = sizeof(outbuf);
        strm.next_out = outbuf;
        retval = inflate(&strm, Z_FINISH);
        switch (retval) {
            case Z_STREAM_ERROR:
                /* state clobbered */
                return 1;
                break;
            case Z_NEED_DICT:
                retval = Z_DATA_ERROR; /* and fall through */
            case Z_DATA_ERROR:
            case Z_MEM_ERROR:
                (void)inflateEnd(&strm);
                return 1;
        }
        
        have = sizeof(outbuf) - strm.avail_out;
        string_concatb(uncompressed, (char *)outbuf, have);
    } while (strm.avail_out == 0);

    (void)inflateEnd(&strm);

    if (retval != Z_STREAM_END) {
        fprintf(stderr, "decompression failed\n");
        return 1;
    }
    
    return 0;
}
