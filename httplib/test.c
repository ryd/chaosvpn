#include <stdio.h>

#include "../string/string.h"
#include "httplib.h"

static void die(char*);

int
main(int argc, char** argv)
{
    struct string inp;
    struct string hostname;
    int port;
    struct string path;

    --argc; ++argv;

    if (!*argv) die("No URL spec'd");
puts(*argv);

    string_initfromstringz(&inp, *argv);
    string_lazyinit(&hostname, 512);
    string_lazyinit(&path, 512);
    http_parseurl(&inp, &hostname, &port, &path);
    printf("HOST: %s | PORT: %d | PATH: %s\n",
            string_get(&hostname),
            port,
            string_get(&path));

}

static void
die(char* msg)
{
    puts(msg);
    exit(1);
}
