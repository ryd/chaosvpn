#include "../string/string.h"
#include "httplib.h"


/**
 * Parse a HTTP URL.
 * @return 0 on success; 1 on invalid URLs; 2 on out of memory errors
 */
int
http_parseurl(struct string* url, struct string* hostname, int* port, struct string* path)
{
    char* s;
    uintptr_t l;
    uintptr_t i;
    int urlpart = 0;
    struct string portnum;
    int retval = HTTP_EOK;

    string_lazyinit(&portnum, 16);
    s = string_get(url);
    l = string_length(url);
    if (l < 7) return HTTP_EINVURL;
    if (memcmp(s, "http://", 7)) { retval=HTTP_EINVURL; goto bail_out; }
    if (!string_concat(path, "/")) { retval=HTTP_ENOMEM; goto bail_out; }
    for (i = 7; i < l; i++) {
        switch(urlpart) {
        case 0:
            switch(s[i]) {
            case ':':
                if (string_length(hostname) < 1) { retval=HTTP_EINVURL; goto bail_out; }
                urlpart = 1;
                continue;

            case '/':
                if (string_length(hostname) < 1) { retval=HTTP_EINVURL; goto bail_out; }
                urlpart = 2;
                continue;

            default:
                if (!string_putc(hostname, s[i])) { retval=HTTP_ENOMEM; goto bail_out; }
            }
            break;

        case 1:
            if (s[i] == '/') { urlpart = 2; break; }
            if ((s[i] < '0') || (s[i] > '9')) { retval=HTTP_EINVURL; goto bail_out; }
            if (!string_putc(&portnum, s[i])) { retval=HTTP_ENOMEM; goto bail_out; }
            break;

        case 2:
            if (!string_concatb(path, s + i, l - i)) { retval=HTTP_ENOMEM; goto bail_out; }
            { retval=HTTP_EOK; goto bail_out_check_port; }
        }
    }

bail_out_check_port:
    if (string_get(&portnum)) {
        if (!string_ensurez(&portnum)) { retval=HTTP_ENOMEM; goto bail_out; }
        *port = atoi(string_get(&portnum));
    } else {
        *port = 80;
    }

bail_out:
    string_free(&portnum);
    return retval;
}
