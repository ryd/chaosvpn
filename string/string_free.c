#include <stdlib.h>

#include "string.h"

void
string_free(struct string* s)
{
	if (s->s) {
		free(s->s);
		s->s = NULL;
		s->size = 0;
		s->length = 0;
	}
}
