#include "ClientStruct.h"
#include "HelloCheak.h"

int TunelSend(ClientSocket* client, int fd, int epfd);

int TunelRecv(ClientSocket* client, int fd, int epfd);