#ifndef __TINC_H
#define __TINC_H

#include "string/string.h"
#define TINC_DEFAULT_PORT "665"
#define TINC_DEFAULT_CIPHER "blowfish"
#define TINC_DEFAULT_COMPRESSION "0"
#define TINC_DEFAULT_DIGEST "sha1"

int tinc_generate_config(struct string*, struct config*);
int tinc_generate_peer_config(struct string*, struct peer_config*);
int tinc_generate_up(struct string*, struct config*);

#endif
