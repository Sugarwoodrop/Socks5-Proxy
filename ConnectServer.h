#include "ClientStruct.h"
#include <time.h>
#include <arpa/inet.h>
#include "HelloCheck.h"

void Connect(ClientSocket* client, int epfd);

void FinishConnect(ClientSocket* client);

void SendDNS(ClientSocket* client, int udpSock);

int CreateDNSpacket(uint8_t* buf, ClientSocket* client);

int encode_dns_name(uint8_t *buf,  uint8_t *domain);

int RecvDNS(int udpSock, ClientSocket** clients, int sizeClients);
