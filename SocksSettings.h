#ifndef SOCKS_SETTINGS_H
#define SOCKS_SETTINGS_H

#include "SendRecvOneByte.h"

int StartSettings(ClientSocket* client);

int SettingsReply(ClientSocket* client);

#endif
