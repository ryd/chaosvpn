#include <stdlib.h>
#include <stdio.h>

#include "string.h"

int main(int argc, char** argv)
{
    struct string args;
    int i; int j; int k;

    string_init(&args, 1, 1);
    string_concatb(&args, "foo", 3);
    string_concatb(&args, "b", 1);
    printf("%d\n", args._u._s.length);
    for (i = 0; i < argc; i++) string_concat(&args, *argv++);
    puts(string_get(&args));
    string_free(&args);

    for (j = 1; j <= 3; j++) {
        for (k = 1; k <= 200; k+=50) {
            printf("Testing with isize=%d growby=%d\n", j, k);
            string_init(&args, j, k);
            for (i = 0; i < 100; i++) {
                string_concat(&args, "test");
            }
            printf("100x test == %d len, %d size, %d grow\n", args._u._s.length,
                    args._u._s.size, args._u._s.growby);
            string_free(&args);
        }
    }

    return 0;
}
