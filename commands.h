#ifndef COMMANDS_H
#define COMMANDS_H

#include "protocol.h"
#include "hashtable.h"

void handle_command(resp_object *cmd, hashtable *store, int client_fd);

#endif