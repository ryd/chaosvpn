#include <stddef.h>
#include <stdio.h>
#include <stdbool.h>

#include "ar.h"

#include "chaosvpn.h"

/*

minimal parsing of ar style archive buffer and in-memory extract

*/

static ssize_t
ar_parseheaderlength(const char *inh, size_t len)
{
  char lintbuf[15];
  ssize_t r;
  char *endp;
  
  if (memchr(inh,0,len)) {
    log_warn("ar_parseheaderlength: contains zero-bytes\n");
    return -1;
  }
  if (len > sizeof(lintbuf)) {
    log_warn("ar_parseheaderlength: length too big\n");
    return -1;
  }
  memcpy(lintbuf,inh,len);
  lintbuf[len]= ' ';
  *strchr(lintbuf, ' ') = '\0';
  r = strtol(lintbuf, &endp, 10);
  if (r < 0) {
    log_warn("ar_parseheaderlength: negative member length\n");
    return -1;
  }
  if (*endp) {
    log_warn("ar_parseheaderlength: contains non-digits\n");
    return -1;
  }
  return r;
}

static bool
ar_compare_name(char *inh, size_t len, char *searchname)
{
  char lintbuf[32];
  
  if (memchr(inh,0,len)) {
    log_warn("ar_compare_name: header contains zero-bytes\n");
    return -1;
  }
  if (len > sizeof(lintbuf)) {
    log_warn("ar_compare_name: length too big\n");
    return -1;
  }
  memcpy(lintbuf,inh,len);
  lintbuf[len]= ' ';
  *strchr(lintbuf, ' ') = '\0';
  if (lintbuf[strlen(lintbuf)-1] == '/') {
    lintbuf[strlen(lintbuf)-1] = '\0';
  }
  
  return (strcmp(lintbuf, searchname) == 0);
}

bool
ar_is_ar_file(struct string *archive)
{
  if (string_length(archive) < SARMAG) {
    log_warn("ar_extract: buffer contents too short\n");
    return false;
  }
  
  if (strncmp(string_get(archive), ARMAG, sizeof(ARMAG)-1) != 0) {
    log_warn("ar_extract: no .ar header at the beginning\n");
    return false;
  }
  
  return true;
}

bool
ar_extract(struct string *archive, char *membername, struct string *result)
{
  ssize_t len;
  char *pos;
  struct ar_hdr *arh;
  ssize_t memberlen;

  string_free(result); // clear result buffer at the start

  len = string_length(archive);
  if (len < SARMAG) {
    log_warn("ar_extract: buffer contents too short\n");
    return false;
  }
  
  pos = string_get(archive);
  
  if (strncmp(pos, ARMAG, sizeof(ARMAG)-1) != 0) {
    log_warn("ar_extract: no .ar header at the beginning\n");
    return false;
  }
  pos += sizeof(ARMAG)-1;
  len -= sizeof(ARMAG)-1;

  for (;;) {
    if (len < sizeof(struct ar_hdr)) {
      log_warn("ar_extract: buffer contents too short\n");
      return false;
    }
    arh = (struct ar_hdr *)pos;
    pos += sizeof(struct ar_hdr);
    len -= sizeof(struct ar_hdr);

    if (memcmp(arh->ar_fmag,ARFMAG,sizeof(arh->ar_fmag))) {
      log_warn("ar_extract: buffer corrupt - bad magic at end of header\n");
      return false;
    }

    memberlen = ar_parseheaderlength(arh->ar_size, sizeof(arh->ar_size));
    if (memberlen < 0) {
      log_warn("ar_extract: buffer corrupt - invalid length field in header\n");
      return false;
    }
    
    if (memberlen+(memberlen&1) > len) {
      log_warn("ar_extract: buffer corrupt - header length bigger than rest of buffer\n");
      return false;
    }
    
    if (ar_compare_name(arh->ar_name, sizeof(arh->ar_name), membername)) {
      // found!
      if (string_concatb(result, pos, memberlen)) {
        log_warn("ar_extract: extract copy failed\n");
        return false;
      }

      return true;
    }

    pos += memberlen+(memberlen&1);
    len -= memberlen+(memberlen&1);

    if (len == 0) {
      // finished, but not found
      return 1;
    } else if (len < 0) {
      // should not happen
      log_warn("ar_extract: buffer corrupt - buffer too short\n");
      return false;
    }
  }
}

/* test program
int
main (int argc,char *argv[]) {

  struct string archive;
  struct string extracted;

  string_init(&archive, 8192, 8192);
  string_init(&extracted, 8192, 1024);

  if (fs_read_file(&archive, "bla.ar")) {
    printf("read error\n");
    exit(1);
  }

  if (ar_extract(&archive, "resolv.conf", &extracted)) {
    printf("ar extract failed\n");
    exit(1);
  }

  string_putc(&extracted, '\0'); // zero-terminate before output!
  printf("contents: %s\n", string_get(&extracted));

  string_free(&archive);
  string_free(&extracted);

  exit(0);
}
*/
