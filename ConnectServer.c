#include "ConnectServer.h"

void Connect(ClientSocket* client, int epfd){
    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(client->settingContext.DSTPORT);

    uint32_t ip =
    ((uint32_t)client->settingContext.addr.IPv4[0] << 24) |
    ((uint32_t)client->settingContext.addr.IPv4[1] << 16) |
    ((uint32_t)client->settingContext.addr.IPv4[2] << 8)  |
    ((uint32_t)client->settingContext.addr.IPv4[3]);

    serv_addr.sin_addr.s_addr = htonl(ip);

    int error = connect(client->socketEndPoint, (struct sockaddr*)&serv_addr, sizeof(serv_addr));
    if (error == 0) {
        client->settingContext.returnStatus = 0x00;
        client->statConnect = C_DONE;
        printf("ПОДКЛЮЧЕНИЕ УСПЕШНО К СЕРВЕРУ\n");
    }
    else if (error < 0) {
        if (errno == EINPROGRESS) {
            client->statConnect = C_WAIT;
            client->eventsClient = EPOLLIN | EPOLLOUT | EPOLLERR;
            modify_epoll_events(epfd, client->socket,  client->eventsClient);
            return;
        }
        client->settingContext.returnStatus = 0x01;
        client->statConnect = C_DONE;
          printf("[CONNECT] errno=%d (%s), socketEnd %d\n", errno, strerror(errno), client->socketEndPoint);
    }
    client->eventsClient = EPOLLIN | EPOLLOUT;
    modify_epoll_events(epfd, client->socket, client->eventsClient);
}

void FinishConnect(ClientSocket* client){
    int err;
    socklen_t len = sizeof(err);
    if(getsockopt(client->socket, SOL_SOCKET, SO_ERROR, &err, &len)){
        perror("gersockopt failed");
    }
    else if (err != 0) {
        printf("Connect failed: %s\n", strerror(err));
        client->settingContext.returnStatus = 0x01;
        client->statConnect = C_DONE;
    } 
    else {
        client->settingContext.returnStatus = 0x00;
        client->statConnect = C_DONE;
        printf("Connect OK\n");
    }
}

void SendDNS(ClientSocket* client, int udpSock){
    uint8_t buf[512];
    int n = CreateDNSpacket(buf, client);
    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(53);
    inet_pton(AF_INET,"8.8.8.8", &serv_addr.sin_addr);
    int sent = sendto(udpSock, buf, n, 0, (struct sockaddr*)&serv_addr, sizeof(serv_addr));
    if (sent < 0) {
        perror("sendto");
        client->settingContext.addr.StateIPv4 = WAIT;
        return;
    }
    printf("SendDNS: sent %d bytes, id=0x%02x%02x\n", sent, client->IdDNS[0], client->IdDNS[1]);

    for (int i = 0; i < n; ++i) {
        printf("%02x ", buf[i]);
        if ((i & 15) == 15) printf("\n");
    }
    printf("\n");

    client->settingContext.addr.StateIPv4 = DNSWAIT;
}

int CreateDNSpacket(uint8_t* buf, ClientSocket* client){
    int n = 0;
    uint16_t id = client->socket;
    buf[n++] = id >> 8;
    client->IdDNS[0] = id >> 8;
    buf[n++] = id & 0xFF;
    client->IdDNS[1] = id & 0xFF;
    buf[n++] = 0x01;
    buf[n++] = 0x00;
    buf[n++] = 0x00;
    buf[n++] = 0x01;
    buf[n++] = 0x00;
    buf[n++] = 0x00;
    buf[n++] = 0x00;
    buf[n++] = 0x00;
    buf[n++] = 0x00;
    buf[n++] = 0x00;
     
    n += encode_dns_name(buf + n, client->settingContext.addr.domainName);
    buf[n++] = 0x00;
    buf[n++] = 0x01;
    buf[n++] = 0x00;
    buf[n++] = 0x01;
    return n;
}

int encode_dns_name(uint8_t *buf,  uint8_t *domain) {
    int n = 0;
    int label_len_pos = n++; 
    int label_len = 0;

    for (int i = 0; domain[i]; i++) {
        if (domain[i] == '.') {
            buf[label_len_pos] = label_len;
            label_len = 0;

            label_len_pos = n++;
        } else {
            buf[n++] = domain[i];
            label_len++;

            if (label_len > 63) return -1; 
        }
    }

    buf[label_len_pos] = label_len; 
    buf[n++] = 0;

    return n;
}

int RecvDNS(int udpSock, ClientSocket** clients, int sizeClients){
    uint8_t buf[512];
    struct sockaddr_in serv_addr;
    socklen_t addr_len = sizeof(serv_addr);
    int r = recvfrom(udpSock, buf, sizeof(buf), 0, (struct sockaddr*)&serv_addr, &addr_len);
    if (r < 0) { 
        if (errno==EAGAIN||errno==EWOULDBLOCK) 
            return -2; 
        perror("recvfrom"); 
        return ERROR; 
    }

    int numberClient = -1;
    for(int i=0;i<sizeClients;i++){
        if(clients[i]->IdDNS[0]==buf[0] && clients[i]->IdDNS[1]==buf[1]){
            numberClient=i; 
            break;
        }
    }
    if(numberClient==-1){ 
        perror("Client not found"); 
        return ERROR; 
    }

    int n=12; 
    while(n<r && buf[n]!=0) n+=buf[n]+1;
    n+=5;

    uint16_t answerCount=(buf[6]<<8)|buf[7];
    bool foundA=false;

    for(int k=0;k<answerCount;k++){
        if(n+10>r) break;

        if((buf[n]&0xC0)==0xC0) n+=2;
        else{ while(n<r && buf[n]!=0) n+=buf[n]+1; n++; }

        if(n+10>r) break;
        uint16_t type=(buf[n]<<8)|buf[n+1]; n+=2;
        n+=2; 
        n+=4; 
        uint16_t rdlen=(buf[n]<<8)|buf[n+1]; n+=2;

        if(type==1 && rdlen==4 && n+4<=r){
            clients[numberClient]->settingContext.addr.IPv4[0]=buf[n];
            clients[numberClient]->settingContext.addr.IPv4[1]=buf[n+1];
            clients[numberClient]->settingContext.addr.IPv4[2]=buf[n+2];
            clients[numberClient]->settingContext.addr.IPv4[3]=buf[n+3];
            clients[numberClient]->settingContext.addr.StateIPv4 = DONE;
            foundA=true;
            printf("Resolved for client fd=%d -> %u.%u.%u.%u\n",
                   clients[numberClient]->socket,
                   clients[numberClient]->settingContext.addr.IPv4[0],
                   clients[numberClient]->settingContext.addr.IPv4[1],
                   clients[numberClient]->settingContext.addr.IPv4[2],
                   clients[numberClient]->settingContext.addr.IPv4[3]);
            break;
        }
        n+=rdlen;
    }

    if(!foundA){
        fprintf(stderr,"No A record found\n");
        clients[numberClient]->settingContext.addr.StateIPv4 = DONE;
        clients[numberClient]->settingContext.returnStatus=0x01;
    }
    return numberClient;
}
