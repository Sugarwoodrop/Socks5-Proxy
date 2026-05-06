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

#define ERROR -1
#define SUCCES 0

int RecvOneByte(ClientSocket* client, uint8_t* writeIn);
int SendOneByte(ClientSocket* client, uint8_t* sendByte);
