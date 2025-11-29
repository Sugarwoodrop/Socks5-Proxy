#include "SocksSettings.h"

int StartSettings(ClientSocket* client){
    if(client->settingContext.stateClientRequest == R_READ_VER){
        int error = RecvOneBait(client, &client->settingContext.VER);
        if(error == 2){
            client->settingContext.stateClientRequest = R_READ_CMD;
            printf("Settings:\nVER %u\n", client->settingContext.VER);
            if(client->settingContext.VER != 0x05){
                return ERROR;
            }
        }
        else{
            return error;
        }
    }
    if(client->settingContext.stateClientRequest == R_READ_CMD){
        int error = RecvOneBait(client, &client->settingContext.CMD);
        if(error == 2){
            if(client->settingContext.CMD == 0x01){
                client->settingContext.stateClientRequest = R_READ_RSV;
                printf("Settings:\nCMD %u\n", client->settingContext.CMD);
            }
            else{
                client->settingContext.returnStatus = 0x07;
            }
        }
        else{
            return error;
        }
    }
    if(client->settingContext.stateClientRequest == R_READ_RSV){
        int error = RecvOneBait(client, &client->settingContext.RSV);
        if(error == 2){
            if(client->settingContext.RSV != 0x00){
                    client->settingContext.returnStatus = 0x07;
            }
            client->settingContext.stateClientRequest = R_READ_ADDR;
            printf("Settings:\nRSV %u\n", client->settingContext.RSV);

        }
        else{
            return error;
        }
    }
    if(client->settingContext.stateClientRequest == R_READ_ADDR){
        if(client->settingContext.addr.StateReadAddr == A_READ_TYPE){
            int error = RecvOneBait(client, &client->settingContext.addr.TYPE);
            if(error == 2){
                if(client->settingContext.addr.TYPE == 0x01){
                    client->settingContext.addr.StateReadAddr = A_READ_IPv4;
                    printf("Settings: Addr: \nTYPE %u\n", client->settingContext.addr.TYPE);
                }
                if(client->settingContext.addr.TYPE == 0x03){
                    client->settingContext.addr.StateReadAddr = A_READ_lengthDomainName;
                    printf("Settings: Addr: \nTYPE %u\n", client->settingContext.addr.TYPE);
                }
                if(client->settingContext.addr.TYPE == 0x04){
                    client->settingContext.returnStatus = 0x08;
                }
            }
            else{
                return error;
            }
            return SUCCES;
        }
        if(client->settingContext.addr.StateReadAddr == A_READ_IPv4){
            int readBaite = recv(client->socket, client->settingContext.addr.IPv4 + client->settingContext.addr.lengthReadIP, 
                                            4 - client->settingContext.addr.lengthReadIP, 0);
            if (readBaite < 0 && errno == EAGAIN){
                return SUCCES;
            }
            if (readBaite < 0){
                perror("recv");
                return ERROR;
            }
            if (readBaite == 0){
                perror("recv");
                return ERROR;
            }
            if(readBaite > 0){
                client->settingContext.addr.lengthReadIP += readBaite;
            }
            if(client->settingContext.addr.lengthReadIP != 4){
                return SUCCES;
            }
            client->settingContext.addr.StateReadAddr = A_DONE;
            client->settingContext.addr.StateIPv4 = DONE;
            printf("Settings: Addr: \n IPv4 ");
            for(int i = 0; i < 4; i++){
                if(i == 3){
                    printf("%u\n", client->settingContext.addr.IPv4[i]);
                    continue;
                }
                printf("%u.", client->settingContext.addr.IPv4[i]);
            }
            return SUCCES;
        }
        if(client->settingContext.addr.StateReadAddr == A_READ_lengthDomainName){
            int error = RecvOneBait(client, &client->settingContext.addr.lengthDomainName);
            if(error == 2){
                client->settingContext.addr.StateReadAddr = A_READ_domainName;
                printf("Settings: Addr:\n lengthDomainName %u\n", client->settingContext.addr.lengthDomainName);
            }
            else{
                return error;
            }   
            return SUCCES;
        }
        if(client->settingContext.addr.StateReadAddr == A_READ_domainName){
            int readBaite = recv(client->socket, client->settingContext.addr.domainName + client->settingContext.addr.lengthReadDomainName, 
                                client->settingContext.addr.lengthDomainName - client->settingContext.addr.lengthReadDomainName, 0);
            if (readBaite < 0 && errno == EAGAIN){
                return SUCCES;
            }
            if (readBaite < 0){
                perror("recv");
                return ERROR;
            }
            if (readBaite == 0){
                perror("recv");
                return ERROR;
            }
            if(readBaite > 0){
                client->settingContext.addr.lengthReadDomainName += readBaite;
            }
            if(client->settingContext.addr.lengthReadDomainName != client->settingContext.addr.lengthDomainName){
                return SUCCES;
            }
            client->settingContext.addr.StateReadAddr = A_DONE;
            client->settingContext.addr.StateIPv4 = WAIT;
            printf("Settings: Addr:\n Domain ");
            for(int i = 0; i < client->settingContext.addr.lengthDomainName; i++){
                printf("%c", client->settingContext.addr.domainName[i]);
            }
            printf("\n");
            return SUCCES;
        }
        if(client->settingContext.addr.StateReadAddr == A_DONE){
            client->settingContext.stateClientRequest = R_READ_DSTPORT;
            return SUCCES;
        }
    }
    if(client->settingContext.stateClientRequest == R_READ_DSTPORT){
        int error = RecvOneBait(client, &client->settingContext.portBait[client->settingContext.sizeReadPortBait]);
        if(error == 2){
            client->settingContext.sizeReadPortBait++;
            if(client->settingContext.sizeReadPortBait == 2){
                client->settingContext.DSTPORT = (client->settingContext.portBait[0]<<8) | (client->settingContext.portBait[1]);
                client->settingContext.stateClientRequest = R_DONE;
                printf("Settings:\n PORT %u\n", client->settingContext.DSTPORT);
            }
        }
        else{
            return error;
        }
    }
    if(client->settingContext.stateClientRequest == R_DONE){
        printf("R_DONE\n\n\n");
        return 2;
    }
    return SUCCES;
}

int SettingsReply(ClientSocket* client){
    if(client->settingContext.stateResponse == R_REPLY_VER){
        int error = SendOneBait(client, &client->settingContext.VER);
        if(error != 2){
            return error;
        }
        client->settingContext.stateResponse = R_REPLY_STATE;
        return SUCCES;
    }
    if(client->settingContext.stateResponse == R_REPLY_STATE){
        int error = SendOneBait(client, &client->settingContext.returnStatus);
        if(error != 2){
            return error;
        }
        client->settingContext.stateResponse = R_REPLY_RSV;
        return SUCCES;
    }
    if(client->settingContext.stateResponse == R_REPLY_RSV){
        int error = SendOneBait(client, &client->settingContext.RSV);
        if(error != 2){
            return error;
        }
        client->settingContext.stateResponse = R_REPLY_ADDR;
        return SUCCES;
    }
    if(client->settingContext.stateResponse == R_REPLY_ADDR){
        if(client->settingContext.addr.StateReplyAddr == A_REPLAY_TYPE){
            uint8_t type = 0x01;
            int error = SendOneBait(client, &type);
            if(error != 2){
                return error;
            }
            client->settingContext.addr.StateReplyAddr = A_REPLAY_IPv4;
            return SUCCES;
        }
        if(client->settingContext.addr.StateReplyAddr == A_REPLAY_IPv4){
            int sendBait = send(client->socket, client->settingContext.addr.ReplayIPv4 + client->settingContext.addr.lengthReplayIP, 
                                    4-client->settingContext.addr.lengthReplayIP, 0);
            if(sendBait < 0 && errno == EAGAIN){
                return SUCCES;
            }
            if(sendBait <= 0){
                perror("send");
                return ERROR;
            }
            client->settingContext.addr.lengthReplayIP += sendBait;
            if(client->settingContext.addr.lengthReplayIP == 4){
                client->settingContext.addr.StateReplyAddr = A_RDONE;
            }
            return SUCCES;
        }
        if(client->settingContext.addr.StateReplyAddr == A_RDONE){
            client->settingContext.stateResponse = R_REPLY_PORT;
            return SUCCES;
        }
    }
    if(client->settingContext.stateResponse == R_REPLY_PORT){
        int sendBait = send(client->socket, client->settingContext.connectPortBait + client->settingContext.sizeReplyPortBait, 
                                2-client->settingContext.sizeReplyPortBait, 0);
        if(sendBait < 0 && errno == EAGAIN){
            return SUCCES;
        }
        if(sendBait <= 0){
            perror("send");
            return ERROR;
        }
        client->settingContext.sizeReplyPortBait += sendBait;
        if(client->settingContext.sizeReplyPortBait == 2){
            client->settingContext.stateResponse = R_RDONE;
        }
    }
    if(client->settingContext.stateResponse == R_RDONE){
        client->setting = true;
        printf("НАСТРОЙКИ ОТПРАВЛЕНЫ \n\n");
        return 2;
    }
    return SUCCES;
}
