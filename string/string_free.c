#include <stdlib.h>

#include "string.h"

void
string_free(struct string* s) {
	if (s->s) {
		free(s->s);
		s->s = NULL;
		s->_u._s.size = 0;
		s->_u._s.length = 0;
	}
}
