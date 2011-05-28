
#ifndef __ADDRMASK_H
#define __ADDRMASK_H

#include <stdbool.h>
#include <sys/socket.h>
#include <limits.h>

#include "string/string.h"


#define ADDR_V4_BITS	32
#define ADDR_V6_BITS	128
#define ADDR_V4_BYTES	((ADDR_V4_BITS + (CHAR_BIT - 1))/CHAR_BIT)
#define ADDR_V6_BYTES	((ADDR_V6_BITS + (CHAR_BIT - 1))/CHAR_BIT)

#define ADDR_MAX_BYTES	ADDR_V6_BYTES

struct addr_info {
  unsigned char net_bytes[ADDR_MAX_BYTES];	/* network portion */
  unsigned char mask_bytes[ADDR_MAX_BYTES];	/* network mask */
  unsigned char addr_family;			/* AF_XXX */
  unsigned char addr_byte_count;		/* typically, 4 or 16 */
  unsigned char addr_bit_count;			/* optimization */
  unsigned char mask_shift;			/* optimization */
  struct addr_info *next;			/* next entry */
};

/* free a struct addr_info */
extern void addrmask_free(struct addr_info *addrinfo);

/* parse string containing ip/mask into struct addr_info */
extern bool addrmask_parse(struct addr_info *ip, const char *addressmask);

/* create new struct addr_info from string containing ip/mask */
extern struct addr_info *addrmask_init(const char *addressmask);

/* matches an ip or subnet against a struct addr_info */
/* returns matching entry from struct addr_info linked list or NULL */
extern struct addr_info *addrmask_match(struct addr_info *matches, const char *addr);

/* convert struct addr_info back to a string */
extern bool addrmask_to_string(struct string *target, struct addr_info *addr);

/* check if char* contains ip or subnet */
extern bool addrmask_verify_ip(const char *addressmask);
extern bool addrmask_verify_subnet(const char *addressmask);

#endif
