#include <stdio.h>
#include <stdbool.h>
#include <sys/socket.h>
#include <sys/types.h>  
#include <netinet/in.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include "ClientStruct.h"
#include "SendRecvOneBait.h"

#define ERROR -1
#define SUCCES 0

int StartUserVerification(ClientSocket* client);
int HelloReply(ClientSocket* client, int epfd);
void modify_epoll_events(int epfd, int fd, uint32_t events);