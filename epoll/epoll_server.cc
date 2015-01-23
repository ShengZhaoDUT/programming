#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <iostream>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

using namespace std;
#define MAX_EVENTS 500
#define IPADDRESS "127.0.0.1"
#define PORT 1234
#define MAXSIZE 1024
#define FDSIZE 1000

struct myevent_s
{
	int fd;
	int events;
	char buf[MAXSIZE];
	int len, s_offset;
	int status;
} mevents[MAXSIZE + 1];

static int socket_bind(const char *ip, int port);
static void do_epoll(int listenfd);
static void handle_events(int epollfd, struct epoll_event *events, int num, int listenfd);
static void handle_accept(int epollfd, int listenfd);
static void do_read(int epollfd, myevent_s *mev);
static void do_write(int epollfd, myevent_s *mev);
static void add_event(int epollfd, myevent_s *mev);
static void modify_event(int epollfd, myevent_s *mev);
static void delete_event(int epollfd, myevent_s *mev);
static void EventSet(myevent_s *ev, int fd);

int main(int argc, char *argv[])
{
	int port = PORT;
	int listenfd;
    if(argc == 2)
        port = atoi(argv[1]);
    listenfd = socket_bind(IPADDRESS, port);
    do_epoll(listenfd);
    return 0;
}

static void EventSet(myevent_s *ev, int fd)
{
	ev->fd = fd;
	ev->events = 0;
	bzero(ev->buf, sizeof(ev->buf));
	ev->len = 0;
	ev->status = 0;
	ev->s_offset = 0;
}

static int socket_bind(const char *ip, int port)
{
	int listenfd;
	struct sockaddr_in servaddr;
	listenfd = socket(AF_INET, SOCK_STREAM, 0);
	if(listenfd == -1) {
		perror("socket error");
		exit(1);
	}
	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	//inet_pton(AF_INET, ip, &servaddr.sin_addr);
	servaddr.sin_addr.s_addr = INADDR_ANY;
	servaddr.sin_port = htons(port);
	if(bind(listenfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) == -1) {
		perror("bind error");
		exit(1);
	}
}

static void do_epoll(int listenfd)
{
	int epollfd;
	struct epoll_event events[MAX_EVENTS];
	int ret;
	epollfd = epoll_create(FDSIZE);
	EventSet(&mevents[MAXSIZE], listenfd);
	mevents[MAXSIZE].events = EPOLLIN;
	add_event(epollfd, &mevents[MAXSIZE]);
	//add_event(epollfd, listenfd, EPOLLIN);
	for(;;) {
		ret = epoll_wait(epollfd, events, MAX_EVENTS, -1);
		handle_events(epollfd, events, ret, listenfd);
	}
	close(epollfd);
}

static void handle_events(int epollfd, epoll_event *events, int num, int listenfd)
{
	int fd;
	for(int i = 0; i < num; i++) {
		myevent_s *ev = (struct myevent_s *)events[i].data.ptr;
		if((ev->fd == listenfd) && (ev->events & EPOLLIN))
			handle_accept(epollfd, listenfd);
		else if(ev->events & EPOLLIN)
			do_read(epollfd, ev);
		else
			do_write(epollfd, ev);
	}
}

static void handle_accept(int epollfd, int listenfd)
{
	int clifd;
	struct sockaddr_in cliaddr;
	socklen_t cliaddrlen;
	clifd = accept(listenfd, (struct sockaddr*) &cliaddr, &cliaddrlen);
	if(clifd == -1)
		perror("accept error:");
	else {
		printf("accpet a new client: %s: %d\n", inet_ntoa(cliaddr.sin_addr), cliaddr.sin_port);
		do {  
			int i = 0;  
	        for(i = 0; i < MAX_EVENTS; i++) {    
	            if(mevents[i].status == 0)
	                break;
	        }
	        if(i == MAX_EVENTS)    
	        {    
	            fprintf(stderr, "%s:max connection limit[%d].", __func__, MAX_EVENTS);    
	            break;    
	        }    
	        // set nonblocking  
	        int iret = 0;  
	        if((iret = fcntl(clifd, F_SETFL, O_NONBLOCK)) < 0)  
	        {  
	            printf("%s: fcntl nonblocking failed:%d", __func__, iret);  
	            break;  
	        }  
	        // add a read event for receive data    
	        EventSet(&mevents[i], clifd);
	        mevents[i].events = EPOLLIN;
			add_event(epollfd, &mevents[i]);  
	    }while(0);
	}
}

static void do_read(int epollfd, myevent_s *ev)
{
	int nread;
	nread = read(ev->fd, ev->buf, MAXSIZE);
	if(nread == -1) {
		perror("read error");
		close(ev->fd);
		delete_event(epollfd, ev);
	}
	else if(nread == 0) {
		perror("client close.\n");
		close(ev->fd);
		delete_event(epollfd, ev);
	}
	else {
		printf("read message is : %s", ev->buf);
		ev->events = EPOLLOUT;
		modify_event(epollfd, ev);
	}
}

static void do_write(int epollfd, myevent_s *ev)
{
	int nwrite;
	nwrite = write(ev->fd, ev->buf, strlen(ev->buf));
	if (nwrite == -1) {
		perror("write error:");
		close(ev->fd);
		delete_event(epollfd, ev);
	}
	else {
		ev->events = EPOLLIN;
		modify_event(epollfd, ev);
	}
	memset(ev->buf, 0, MAXSIZE);
}

static void add_event(int epollfd, myevent_s *mev)
{
	struct epoll_event ev = {0, {0}};
	ev.events = mev->events;
	ev.data.ptr = mev;
	epoll_ctl(epollfd, EPOLL_CTL_ADD, mev->fd, &ev);
}

static void delete_event(int epollfd, myevent_s *mev)
{
	struct epoll_event ev = {0, {0}};
	ev.events = mev->events;
	ev.data.ptr = mev;
	epoll_ctl(epollfd, EPOLL_CTL_DEL, mev->fd, &ev);
}

static void modify_event(int epollfd, myevent_s *mev)
{
	struct epoll_event ev = {0, {0}};
	ev.events = mev->events;
	ev.data.ptr = mev;
	epoll_ctl(epollfd, EPOLL_CTL_MOD, mev->fd, &ev);
}