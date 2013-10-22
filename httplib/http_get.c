#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>   
#include <unistd.h>

#ifdef WIN32
#ifndef WINVER
#define WINVER WindowsXP
#endif

#include <w32api.h>
#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>
#else
#include <sys/socket.h>
#include <netdb.h>
#endif

#include "../string/string.h"
#include "httplib.h"

static int epoch2http(struct string*, time_t);
static int sendall(int, void*, size_t, int);
static int httprecv(int, struct string*, int*);
static int handle_header(struct string*, int*);

/* TODO: make configurable somehow (nicely!) */
#define GENERIC_SOCKET_TIMEOUT 10


/**
 * Fetch a URL
 * @param url URL to fetch
 * @param buffer buffer to fetch data into
 * @param ifmodifiedsince
 * @param useragent
 * @param servererror
 * @param errormessage
 * @return zero on success, else errval (see HTTP_*-consts)
 */
int
http_get(struct string* url, struct string* buffer,
        time_t ifmodifiedsince, struct string* useragent,
        int* servererror, struct string* /* unused - TBI */ errormessage)
{
    struct string hostname;
    struct string path;
    struct string request;
    int port;
    int sfd;
    int retval = HTTP_EOK;
    struct addrinfo hints, *res, *rp;
    struct string ims;
    char s_port[16];
    struct timeval tv;

    tv.tv_sec = GENERIC_SOCKET_TIMEOUT;
    tv.tv_usec = 0;

    string_init(&hostname, 4096, 4096);
    string_init(&path, 4096, 4096);
    if ((retval = http_parseurl(url, &hostname, &port, &path))) {
        goto bail_out_free_hostname;
    }

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = 0;
    hints.ai_protocol = 0;

    if (!string_ensurez(&hostname)) {
        retval = HTTP_ENOMEM;
        goto bail_out_free_hostname;
    }
    snprintf(s_port, 16, "%d", port);
    if (getaddrinfo(string_get(&hostname), s_port, &hints, &res)) {
        retval = HTTP_ENETERR;
        goto bail_out_free_hostname;
    }

    for (rp = res; rp != NULL; rp = rp->ai_next) {
        sfd = socket(rp->ai_family, rp->ai_socktype,
                rp->ai_protocol);
        if (sfd == -1) {
            continue;
        }
        (void)setsockopt(sfd, SOL_SOCKET, SO_SNDTIMEO, (char*) &tv, sizeof(tv));
        (void)setsockopt(sfd, SOL_SOCKET, SO_RCVTIMEO, (char*) &tv, sizeof(tv));
        if (connect(sfd, rp->ai_addr, rp->ai_addrlen) != -1) {
            break;
        }
        close(sfd);
    }
    freeaddrinfo (res);
    if (rp == NULL) {
        retval = HTTP_ENETERR;
        goto bail_out_free_hostname;
    }

    string_init(&request, 4096, 4096);
    string_concat_sprintf(&request, "GET %S HTTP/1.1\r\nHost: %S\r\nUser-Agent: %S\r\n",
            &path, &hostname, useragent);
    if (ifmodifiedsince) {
        string_init(&ims, 512, 512);
        if (epoch2http(&ims, ifmodifiedsince)) {
            string_free(&ims);
            retval = HTTP_ENOMEM;
            goto bail_out;
        }
        if (!string_concat_sprintf(&request, "If-Modified-Since: %S\r\n", &ims)) {
            string_free(&ims);
            retval = HTTP_ENOMEM;
            goto bail_out;
        }
        string_free(&ims);
    }
    if (!string_concat(&request, "Connection: close\r\n")) { retval=HTTP_ENOMEM; goto bail_out; };
    if (!string_concat(&request, "\r\n")) { retval=HTTP_ENOMEM; goto bail_out; }

    if (sendall(sfd, request.s, request._u._s.length, 0)) {
        retval = HTTP_ENETERR;
        goto bail_out;
    }

    if ((retval = httprecv(sfd, buffer, servererror))) {
        goto bail_out;
    }

bail_out:
    close (sfd);
    string_free(&request);
bail_out_free_hostname:
    string_free(&hostname);
    string_free(&path);
    return retval;
}

static int
httprecv(int sfd, struct string* buf, int* httpres)
{
    char* b;
    ssize_t bl, bptr = 0;
    size_t i;
    struct string oneline;
    int retval = HTTP_EOK;
    int BUFSIZE = 8192;
    int isfirsthdr = 1;

    if ((b = malloc(BUFSIZE)) == NULL) return HTTP_ENOMEM;
    string_init(&oneline, 512, 512);

    while(1) {
        bl = recv(sfd, b + bptr, BUFSIZE - bptr, 0);
        if (bl == 0) {
            if (bptr == 0) {
                break;
            }
        } else if (bl < 0) {
            retval = HTTP_ENETERR;
            goto bail_out;
        }
        bptr += bl;
        if ((*b == '\n') || (*b == '\r')) goto finished;
        for (i = 0; i < bptr; i++) {
            if (b[i] == '\n') {
                string_clear(&oneline);
                if (!string_concatb(&oneline, b, i)) { retval=HTTP_ENOMEM; goto bail_out; }
                if (oneline.s[oneline._u._s.length - 1] == '\r') --oneline._u._s.length;
                if (++i >= bptr) { retval=HTTP_ESRVERR; goto bail_out; }
                if (bptr != i) {
                    memmove(b, b + i, bptr - i);
                }
                bptr -= i;
                if (isfirsthdr) {
                    if (handle_header(&oneline, httpres)) { retval=HTTP_ESRVERR; goto bail_out; }
                    isfirsthdr = 0;
                }
                break;
            }
        }
    }
finished:

    if (isfirsthdr) { retval=HTTP_ESRVERR; goto bail_out; }

    bl = 0;
    if (bptr > 2) {
        if ((b[0] == '\r') || (b[0] == '\n')) {
            ++bl;
            if ((b[1] == '\r') || (b[1] == '\n')) {
                ++bl;
            }
        }
    }
    if (!string_concatb(buf, b + bl, bptr - bl)) { retval=HTTP_ENOMEM; goto bail_out; }
    while(1) {
        bl = recv(sfd, b, BUFSIZE, 0);
        if (bl == 0) break;
        if (bl < 0) { retval=HTTP_ENETERR; goto bail_out; }
        if (!string_concatb(buf, b, bl)) { retval=HTTP_ENOMEM; goto bail_out; }
    }

    if (retval == HTTP_EOK) if (*httpres != 200) retval = HTTP_ESRVERR;

bail_out:
    close(sfd);
    string_free(&oneline);
    free(b);
    return retval;
}

static int
handle_header(struct string* s, int* httpres)
{
    char* b;
    uintptr_t l, i;
    char* p, * p2;

    b = string_get(s);
    l = string_length(s);
    if (l < 4) return HTTP_ENETERR;
    for (i = 0; i < l; i++) if (b[i] == 0) return HTTP_ENETERR;
    if (!string_putc(s, 0)) return HTTP_ENOMEM;
    if (memcmp(b, "HTTP", 4)) return HTTP_ENETERR;
    if ((p = strchr(b, ' ')) == NULL) return HTTP_ENETERR;
    if ((p2 = strchr(p + 1, ' ')) == NULL) return HTTP_ENETERR;
    *p2 = 0;
    *httpres = atoi(p);
    *p2 = 32;
    return HTTP_EOK;
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
    return HTTP_EOK;
}

static int
epoch2http(struct string* s, time_t time)
{
    char buf[512];
    struct tm* tm;

    tm = gmtime(&time);
    strftime(buf, 512, "%a, %d %b %Y %H:%M:%S GMT", tm);
    return(!string_concat(s, buf));
}
