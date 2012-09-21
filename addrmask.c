#include <string.h>
#include <sys/socket.h>
#ifndef WIN32
#include <arpa/inet.h>
#endif

#include "chaosvpn.h"
#include "addrmask.h"


/* needs to be included late for BSD support */
#include <sys/types.h>

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
  struct addr_info *next;

  while (addrinfo) {
    next = addrinfo->next;
    free(addrinfo);
    addrinfo = next;
  }
}

bool addrmask_parse(struct addr_info *ip, const char *addressmask)
{
  bool res;
  struct string patternstruct;
  char *pattern;
  char *mask_search;
  char *mask;
  unsigned char *np;
  unsigned char *mp;
  struct addrinfo hints = {
    .ai_flags = AI_NUMERICHOST,
    .ai_socktype = SOCK_STREAM,
    .ai_protocol = 0,
    };
  struct addrinfo *ai = NULL;
  void *addrbytes;


  res = false;
  if (!ip)
    goto out;
  memset(ip, 0, sizeof(struct addr_info));

  if (string_initfromstringz(&patternstruct, addressmask))
    goto out;
  if (string_putc(&patternstruct, 0))
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
    if ((mask_search = str_split_at(pattern, ']')) == NULL)
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
  if ((mask = str_split_at(mask_search, '/')) != NULL) {
    ip->addr_family = CIDR_MATCH_ADDR_FAMILY(pattern);
    ip->addr_bit_count = CIDR_MATCH_ADDR_BIT_COUNT(ip->addr_family);
    ip->addr_byte_count = CIDR_MATCH_ADDR_BYTE_COUNT(ip->addr_family);
    hints.ai_family = ip->addr_family;

    if (!str_alldig(mask)
        || (ip->mask_shift = atoi(mask)) > ip->addr_bit_count
	|| getaddrinfo(pattern, "0", &hints, &ai) != 0 || !ai) {
      goto out_free_pattern;
    }

    if (ip->addr_family == AF_INET) {
      struct sockaddr_in *sock4 = (struct sockaddr_in *) ai->ai_addr;
      addrbytes = &sock4->sin_addr.s_addr;
    } else if (ip->addr_family == AF_INET6) {
      struct sockaddr_in6 *sock6 = (struct sockaddr_in6 *) ai->ai_addr;
      addrbytes = &sock6->sin6_addr.s6_addr;
    } else {
      goto out_free_pattern;
    }
    memcpy(ip->net_bytes, addrbytes, ip->addr_byte_count);

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
#ifndef WIN32
	char dummy[255];
        if (inet_ntop(ip->addr_family, ip->net_bytes, dummy, sizeof(dummy)) == NULL)
          goto out_free_pattern;
#endif
      }
    }
  } else {
    /* No /mask specified. Treat a bare network address as /allbits. */
    ip->addr_family = CIDR_MATCH_ADDR_FAMILY(pattern);
    ip->addr_bit_count = CIDR_MATCH_ADDR_BIT_COUNT(ip->addr_family);
    ip->addr_byte_count = CIDR_MATCH_ADDR_BYTE_COUNT(ip->addr_family);
    hints.ai_family = ip->addr_family;

    if (getaddrinfo(pattern, "0", &hints, &ai) != 0 || !ai) {
      goto out_free_pattern;
    }

    if (ip->addr_family == AF_INET) {
      struct sockaddr_in *sock4 = (struct sockaddr_in *) ai->ai_addr;
      addrbytes = &sock4->sin_addr.s_addr;
    } else if (ip->addr_family == AF_INET6) {
      struct sockaddr_in6 *sock6 = (struct sockaddr_in6 *) ai->ai_addr;
      addrbytes = &sock6->sin6_addr.s6_addr;
    } else {
      goto out_free_pattern;
    }
    memcpy(ip->net_bytes, addrbytes, ip->addr_byte_count);

    ip->mask_shift = ip->addr_bit_count;
    /* Allow for bytes > 8. */
    memset(ip->mask_bytes, (unsigned char) -1, ip->addr_byte_count);
  }
    
  res = true;

out_free_pattern:
  if (ai) {
    freeaddrinfo(ai);
  }
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
  struct addr_info info;
  unsigned char *mp;
  unsigned char *np;
  unsigned char *ap;
  struct addr_info *entry;

  if (!matches || !addr)
    return NULL;

  if (!addrmask_parse(&info, addr))
    return NULL;

  for (entry = matches; entry; entry = entry->next) {
    if (entry->addr_family == info.addr_family) {
      /* Unoptimized case: netmask with some or all bits zero. */
      if (entry->mask_shift < entry->addr_bit_count) {
        for (np = entry->net_bytes, mp = entry->mask_bytes,
            ap = info.net_bytes; /* void */ ; np++, mp++, ap++) {
          if (ap >= info.net_bytes + entry->addr_byte_count)
            goto found;
          if ((*ap & *mp) != *np)
            break;
        }
      }
      
      /* Optimized case: all 1 netmask (i.e. no netmask specified). */
      else {
        for (np = entry->net_bytes,
            ap = info.net_bytes; /* void */ ; np++, ap++) {
          if (ap >= info.net_bytes + entry->addr_byte_count)
            goto found;
          if (*ap != *np)
            break;
        }
      }
    }
  }
  
  return NULL;

found:
  /* Address matches, now check for subnet size */
  
  if (info.mask_shift < entry->mask_shift)
    return NULL;
  
  return entry;
}

bool addrmask_to_string(struct string *target, struct addr_info *addr)
{
  char buffer[255];
  int res;
  struct sockaddr_storage sock;

  if (!target || !addr)
    return false;

  memset(&sock, 0, sizeof(sock));
  if (addr->addr_family == AF_INET) {
    struct sockaddr_in *sock4 = (struct sockaddr_in *) &sock;

    sock4->sin_family = addr->addr_family;
    memcpy(&sock4->sin_addr.s_addr, addr->net_bytes, addr->addr_byte_count);
  } else if (addr->addr_family == AF_INET6) {
    struct sockaddr_in6 *sock6 = (struct sockaddr_in6 *) &sock;

    sock6->sin6_family = addr->addr_family;
    memcpy(sock6->sin6_addr.s6_addr, addr->net_bytes, addr->addr_byte_count);
  } else {
    return false;
  }

  res = getnameinfo((struct sockaddr *) &sock, sizeof(sock), buffer, sizeof(buffer), NULL, 0, NI_NUMERICHOST);
  if (res != 0)
    return false;

  if (string_concat(target, buffer))
    return false;
  if (string_putc(target, '/'))
    return false;
  
  if (string_putint(target, addr->mask_shift))
    return false;

  return true;
}

bool addrmask_verify_subnet(const char *addressmask, const unsigned short int family)
{
  struct addr_info *addr;
  bool res;

  if (str_is_empty(addressmask))
    return false;

  addr = addrmask_init(addressmask);
  if (addr == NULL)
    return false;

  if (family == AF_INET && addr->addr_family == AF_INET) {
    res = true;
  } else if (family == AF_INET6 && addr->addr_family == AF_INET6) {
    res = true;
  } else if (family == AF_UNSPEC) {
    res = true;
  } else {
    res = false;
  }

  addrmask_free(addr);
  return res;
}

bool addrmask_verify_ip(const char *addressmask, const unsigned short int family)
{
  struct addr_info *addr;
  bool res = false;

  if (str_is_empty(addressmask))
    return false;

  addr = addrmask_init(addressmask);
  if (addr == NULL)
    return false;

  if ((addr->addr_family == AF_INET) && (addr->mask_shift == 32)) {
    res = true;
  } else if ((addr->addr_family == AF_INET6) && (addr->mask_shift == 128)) {
    res = true;
  }

  if (res) {
    if (family == AF_INET && addr->addr_family == AF_INET) {
      res = true;
    } else if (family == AF_INET6 && addr->addr_family == AF_INET6) {
      res = true;
    } else if (family == AF_UNSPEC) {
      res = true;
    } else {
      res = false;
    }
  }

  addrmask_free(addr);
  return res;
}
