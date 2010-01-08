#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>

int
main ()
{
  struct addrinfo hints, *res;
  int sockfd, status, error, hdr;
  char getreq[] = "GET /tinc-chaosvpn.txt HTTP/1.0\r\n\r\n";
  int buf_size = 16;
  char *buf = NULL;
  FILE *conf = stdout;
  int read_size, out_size = 0;

  buf = calloc (1, buf_size);
  memset (&hints, 0, sizeof (hints));
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  error = getaddrinfo ("127.0.0.1", "http", &hints, &res);

  if (error != 0)
    {
      fprintf (stderr, "Error in getaddrinfo: %s\n", gai_strerror (error));
      return 1;
    }

  freeaddrinfo (res);

  sockfd = socket (res->ai_family, res->ai_socktype, res->ai_protocol);
  if (sockfd == -1)
    {
      perror ("socket");
    }
  else
    {

      if (connect (sockfd, res->ai_addr, res->ai_addrlen) == 0);
      send (sockfd, getreq, strlen (getreq), 0);
    }

  conf = fopen ( /*"/etc/tinc/ */ "tinc-chaosvpn.txt", "w+");
  while ((read_size = read (sockfd, buf, buf_size)) > 0)
    {
      buf[read_size] = '\0';
      fputs (buf, conf);
      out_size += read_size;
      memset (buf, buf_size, 0);
    }
  fprintf (stdout, "Received  %d bytes\n", out_size);
  close (sockfd);
  fclose (conf);
  return 0;
}
