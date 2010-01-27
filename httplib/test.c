#include <stdio.h>

#include "../string/string.h"
#include "httplib.h"

static void die(char*);

int
main(int argc, char** argv)
{
    struct string buffer;
    struct string inp;
    struct string em;

    --argc; ++argv;

    if (!*argv) die("No URL spec'd");
puts(*argv);

    string_initfromstringz(&inp, *argv);
    string_lazyinit(&buffer, 8192);
    string_lazyinit(&em, 512);
    printf("calling geturl: %d\n",
            http_get(&inp, &buffer, 1264589818, &em, &em));
    printf("Res: %s\n", buffer.s);

}

static void
die(char* msg)
{
    puts(msg);
    exit(1);
}
