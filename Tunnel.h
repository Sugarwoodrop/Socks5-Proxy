#ifndef TUNNEL_H
#define TUNNEL_H

#include "ClientStruct.h"
#include "HelloCheck.h"

int TunnelSend(ClientSocket* client, int fd, int epfd);

int TunnelRecv(ClientSocket* client, int fd, int epfd);
#endif
