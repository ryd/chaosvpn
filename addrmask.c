
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include "chaosvpn.h"
#include "addrmask.h"

#define CIDR_MATCH_ADDR_FAMILY(a) (strchr((a), ':') ? AF_INET6 : AF_INET)
#define CIDR_MATCH_ADDR_BIT_COUNT(f) \
  ((f) == AF_INET6 ? ADDR_V6_BITS : \
   (f) == AF_INET ? ADDR_V4_BITS : \
   (/* error */ ADDR_V6_BITS))
#define CIDR_MATCH_ADDR_BYTE_COUNT(f) \
  ((f) == AF_INET6 ? ADDR_V6_BYTES : \
   (f) == AF_INET ? ADDR_V4_BYTES : \
   (/* error */ ADDR_V6_BYTES))


static void mask_addr(unsigned char *addr_bytes, unsigned addr_byte_count, unsigned network_bits)
{
  unsigned char *p;

  if (network_bits > addr_byte_count * CHAR_BIT)
    return;

  p = addr_bytes + network_bits / CHAR_BIT;
  network_bits %= CHAR_BIT;

  if (network_bits != 0)
    *p++ &= ~0 << (CHAR_BIT - network_bits);

  while (p < addr_bytes + addr_byte_count)
    *p++ = 0;
}

void addrmask_free(struct addr_info *addrinfo)
{
  if (addrinfo == NULL)
    return;

  if (addrinfo->next != NULL)
    addrmask_free(addrinfo->next);
  
  free(addrinfo);
}

bool addrmask_parse(struct addr_info *ip, const char *addressmask)
{
  bool res;
  struct string patternstruct;
  char *pattern;
  char *mask_search;
  char *mask;
  char dummy[255];
  unsigned char *np;
  unsigned char *mp;

  res = false;
  if (!ip)
    goto out;
  ip->next = NULL;

  if (string_initfromstringz(&patternstruct, addressmask))
    goto out;
  pattern = string_get(&patternstruct);


  /*
   * Strip [] from [addr/len] or [addr]/len, destroying the pattern. CIDR
   * maps don't need [] to eliminate syntax ambiguity, but matchlists need
   * it. While stripping [], figure out where we should start looking for
   * /mask information.
   */
  if (*pattern == '[') {
    pattern++;
    if ((mask_search = str_split_at(pattern, ']')) == 0)
      goto out_free_pattern;
    else if (*mask_search != '/') {
      if (*mask_search != 0)
        goto out_free_pattern;
      mask_search = pattern;
    }
  } else {
    mask_search = pattern;
  }

  /* Parse the pattern into network and mask, destroying the pattern. */
  if ((mask = str_split_at(mask_search, '/')) != 0) {
    ip->addr_family = CIDR_MATCH_ADDR_FAMILY(pattern);
    ip->addr_bit_count = CIDR_MATCH_ADDR_BIT_COUNT(ip->addr_family);
    ip->addr_byte_count = CIDR_MATCH_ADDR_BYTE_COUNT(ip->addr_family);
    if (!str_alldig(mask)
        || (ip->mask_shift = atoi(mask)) > ip->addr_bit_count
        || inet_pton(ip->addr_family, pattern, ip->net_bytes) != 1) {
      goto out_free_pattern;
    }
    if (ip->mask_shift > 0) {
      /* Allow for bytes > 8. */
      memset(ip->mask_bytes, (unsigned char) -1, ip->addr_byte_count);
      mask_addr(ip->mask_bytes, ip->addr_byte_count, ip->mask_shift);
    } else {
      memset(ip->mask_bytes, 0, ip->addr_byte_count);
    }
    
    /* Sanity check: all host address bits must be zero. */
    for (np = ip->net_bytes, mp = ip->mask_bytes;
        np < ip->net_bytes + ip->addr_byte_count; np++, mp++) {
      if (*np & ~(*mp)) {
        mask_addr(ip->net_bytes, ip->addr_byte_count, ip->mask_shift);
        if (inet_ntop(ip->addr_family, ip->net_bytes, dummy,
            sizeof(dummy)) == 0)
          goto out_free_pattern;
      }
    }
  } else {
    /* No /mask specified. Treat a bare network address as /allbits. */
    ip->addr_family = CIDR_MATCH_ADDR_FAMILY(pattern);
    ip->addr_bit_count = CIDR_MATCH_ADDR_BIT_COUNT(ip->addr_family);
    ip->addr_byte_count = CIDR_MATCH_ADDR_BYTE_COUNT(ip->addr_family);
    if (inet_pton(ip->addr_family, pattern, ip->net_bytes) != 1) {
      goto out_free_pattern;
    }
    ip->mask_shift = ip->addr_bit_count;
    /* Allow for bytes > 8. */
    memset(ip->mask_bytes, (unsigned char) -1, ip->addr_byte_count);
  }
    
  res = true;

out_free_pattern:
  string_free(&patternstruct);
out:
  return res;
}

struct addr_info *addrmask_init(const char *addressmask)
{
  struct addr_info *result;
  
  result = malloc(sizeof(struct addr_info));
  if (!result)
    return NULL;
  memset(result, 0, sizeof(struct addr_info));
  
  if (!addrmask_parse(result, addressmask)) {
    free(result);
    return NULL;
  }
  
  return result;
}

struct addr_info *addrmask_match(struct addr_info *matches, const char *addr)
{
  unsigned char addr_bytes[ADDR_MAX_BYTES];
  unsigned addr_family;
  unsigned char *mp;
  unsigned char *np;
  unsigned char *ap;
  struct addr_info *entry;

  if (!matches)
    return NULL;
  
  addr_family = CIDR_MATCH_ADDR_FAMILY(addr);
  if (inet_pton(addr_family, addr, addr_bytes) != 1)
    return NULL;
  
  for (entry = matches; entry; entry = entry->next) {
    if (entry->addr_family == addr_family) {
      /* Unoptimized case: netmask with some or all bits zero. */
      if (entry->mask_shift < entry->addr_bit_count) {
        for (np = entry->net_bytes, mp = entry->mask_bytes,
            ap = addr_bytes; /* void */ ; np++, mp++, ap++) {
          if (ap >= addr_bytes + entry->addr_byte_count)
            return (entry);
          if ((*ap & *mp) != *np)
            break;
        }
      }
      
      /* Optimized case: all 1 netmask (i.e. no netmask specified). */
      else {
        for (np = entry->net_bytes,
            ap = addr_bytes; /* void */ ; np++, ap++) {
          if (ap >= addr_bytes + entry->addr_byte_count)
            return (entry);
          if (*ap != *np)
            break;
        }
      }
    }
  }
  
  return NULL;
}

bool addrmask_to_string(struct string *target, struct addr_info *addr)
{
  char buffer[255];
  const char *res;

  if (!target || !addr)
    return false;

  res = inet_ntop(addr->addr_family, addr->net_bytes, buffer, sizeof(buffer));
  if (!res)
    return false;

  if (string_concat(target, buffer))
    return false;
  if (string_putc(target, '/'))
    return false;
  
  if (string_putint(target, addr->mask_shift))
    return false;

  return true;
}
