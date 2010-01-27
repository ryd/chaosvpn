#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <time.h>
#include <sys/socket.h>

#include "../string/string.h"

static int epoch2http(struct string*, time_t);
static int sendall(int, void*, size_t, int);

/**
 * Fetch an URL
 * @param url URL to fetch
 * @param buffer buffer to fetch data into
 * @param ifmodifiedsince
 * @param servererror
 * @param errormessage
 * @return 0 on success, 1 on url format errors, 2 on memory errors, 3 on network errors, 4 on server errors
 */
int
http_get(struct string* url, struct string* buffer, time_t ifmodifiedsince, int* servererror, struct string* errormessage)
{
    struct string hostname;
    struct string path;
    struct string request;
    int port;
    int sfd;
    int retval = 0;
    struct addrinfo hints, *res, *rp;
    struct string ims;

    string_init(&hostname, 4096, 4096);
    string_init(&path, 4096, 4096);
    if ((retval = http_parseurl(url, &hostname, &port, &path))) {
        string_free(&hostname);
        string_free(&path);
        return retval;
    }

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = 0;
    hints.ai_protocol = 0;

    if ((retval = getaddrinfo (string_get(&hostname), "http", &hints, &res))) {
        return retval;
    }

    for (rp = res; rp != NULL; rp = rp->ai_next) {
        sfd = socket(rp->ai_family, rp->ai_socktype,
                rp->ai_protocol);
        if (sfd == -1) {
            continue;
        }
        if (connect(sfd, rp->ai_addr, rp->ai_addrlen) != -1) {
            break;
        }
        close(sfd);
    }
    freeaddrinfo (res);
    if (rp == NULL) {
        string_free(&hostname);
        string_free(&path);
        return 3;
    }

    string_init(&request, 4096, 4096);
    string_concat_sprintf(&request, "GET %S HTTP/1.1\r\nHost: %S\r\n",
            &path, &hostname);
    if (ifmodifiedsince) {
        string_init(&ims, 512, 512);
        if (epoch2http(&ims, ifmodifiedsince)) {
            retval = 2;
            goto bail_out;
        }
        string_concat_sprintf(&request, "If-Modified-Since: %S\r\n", &ims);
    }
    string_concat(&request, "\r\n");

    if (sendall(sfd, request.s, request._u._s.length, MSG_NOSIGNAL)) {
        retval = 3;
        goto bail_out;
    }

bail_out:
    close (sfd);
    string_free(&hostname);
    string_free(&path);
    string_free(&request);
    return retval;
}

static int
sendall(int sfd, void* buf, size_t len, int flags)
{
    size_t bas = 0;
    ssize_t res;
    while (len) {
        res = send(sfd, buf + bas, len, flags);
        if (res < 0) {
            return res;
        }
        bas += res; len -= res;
    }
    return 0;
}

static int
epoch2http(struct string* s, time_t time)
{
    char buf[512];
    struct tm* tm;

    tm = localtime(&time);
    strftime(buf, 512, "%a, %d %b %Y %H:%M:%S %Z", tm);
    return(string_concat(s, buf));
}
