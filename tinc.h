#ifndef __TINC_H
#define __TINC_H

#include "string/string.h"
#define TINC_DEFAULT_PORT "665"
/*
   ^^ Note: This 665 is a typo, it should have been 655 instead.
            But fixing this may mean creating imcompatibiliies between
            older versions of this program and current versions, peers
            using the default port would have to change their firewall
            rules - so just keep it.
            (The ancient perl version correctly used 655)
 */

#define TINC_DEFAULT_CIPHER "blowfish"
#define TINC_DEFAULT_COMPRESSION "0"
#define TINC_DEFAULT_DIGEST "sha1"

extern int tinc_generate_config(struct string*, struct config*);
extern int tinc_generate_peer_config(struct string*, struct peer_config*);
extern int tinc_generate_updown(struct config*, bool up);
extern char *tinc_get_version(struct config *config);

#endif
