#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
	
int 
main() 
{
	struct addrinfo hints, *res;
	int sockfd, status, error, conn, ret;
	char getreq[]= "GET /tinc-chaosvpn.txt HTTP/1.0\r\n\r\n";
	char buf[25000];
	FILE *conf;
	char confile[] = "tinc-chaosvpn.txt";	

	memset(&hints, 0, sizeof(hints));
        hints.ai_family = AF_UNSPEC; 
        hints.ai_socktype = SOCK_STREAM;
        error = getaddrinfo("127.0.0.1", "http", &hints, &res);

        if (error != 0) {
           fprintf(stderr, "Error in getaddrinfo: %s\n", gai_strerror(error));
           return 1;
        }
	
	freeaddrinfo(res);
	
	sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
	if (sockfd == -1) {
	   perror("socket");
	}
	else {
	   
	if (connect(sockfd, res->ai_addr, res->ai_addrlen) == 0);
	      send(sockfd, getreq, strlen(getreq), 0);
	      status = recv(sockfd, buf, sizeof(buf), 0);
	      fprintf(stdout, "Received  %d bytes\n", status);
	      conf = fopen(confile, "w+");
	      fputs(buf, conf);
	      fclose(conf);
	      close(sockfd);
       }

return 0;
}
 	   
