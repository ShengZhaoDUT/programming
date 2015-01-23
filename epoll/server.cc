#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <arpa/inet.h>

int main(int argc, char *argv[])
{
 int sockfd;
 int newsockfd;
 unsigned int clilen;
 char buffer[256];
 struct sockaddr_in serv_addr;
 struct sockaddr_in cli_addr;
 struct in_addr in_cli_addr;
 int n;
 
 sockfd = socket(AF_INET, SOCK_STREAM, 0);
 if (sockfd < 0)
 {
  printf("opening socket error! \n");
  exit(-1);
 }
 bzero((char *)&serv_addr, sizeof(serv_addr));
 serv_addr.sin_family = AF_INET;
 serv_addr.sin_addr.s_addr = INADDR_ANY;
 serv_addr.sin_port = htons(atoi(argv[1]));
 if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
 {
  printf("socket binding error! \n");
  exit(-1);
 }
 
 listen(sockfd, 5);
 clilen = sizeof(cli_addr);
 newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
 if (newsockfd < 0)
 {
  printf("accept error! \n");
  exit(-1);
 }
 
 in_cli_addr.s_addr = cli_addr.sin_addr.s_addr;
 printf("client ip address : %s \n", inet_ntoa(in_cli_addr));
 
 bzero(buffer, 256);
 n = read(newsockfd, buffer, sizeof(buffer));
 if (n < 0)
  printf("error reading from socket! \n");
 printf("here is the message : %s \n", buffer);
 strcat(buffer, "server return");
 n = write(newsockfd, buffer, sizeof(buffer));
 if (n < 0)
   printf("error writing to socket");
 return 0;
}