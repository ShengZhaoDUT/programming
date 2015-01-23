#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define MAXLINE 1024

int main(int argc, char *argv[])
{
	int sockfd;
	int n;
	struct sockaddr_in serv_addr;
	char buffer[MAXLINE];
	
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if(sockfd < 0) {
		perror("opening sock error.");
		exit(1);
	}
	bzero(&serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(atoi(argv[2]));
	//inet_pton(AF_INET, argv[1], &serv_addr.sin_addr);
	serv_addr.sin_addr.s_addr = inet_addr(argv[1]);
	if(connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
		perror("connecting error");
		exit(1);
	}

	bzero(buffer, MAXLINE);
	fgets(buffer, MAXLINE, stdin);

	n = write(sockfd, buffer, strlen(buffer));
	if(n < 0)
		perror("error to write socket");
	bzero(buffer, MAXLINE);
	n = read(sockfd, buffer, MAXLINE);
	if(n < 0)
		perror("error to read socket");
	printf("%s\n", buffer);
	exit(0);
}