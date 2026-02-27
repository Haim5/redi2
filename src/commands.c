#include "commands.h"
#include <string.h>
#include <stdio.h>

static void send_simple_string(int fd, const char *s) {
    char buf[1024];
    int len = snprintf(buf, sizeof(buf), "+%s\r\n", s);
    write(fd, buf, len);
}

static void send_error(int fd, const char *s) {
    char buf[1024];
    int len = snprintf(buf, sizeof(buf), "-%s\r\n", s);
    write(fd, buf, len);
}

static void send_bulk_string(int fd, const char *s) {
    if (!s) {
        write(fd, "$-1\r\n", 5);
        return;
    }
    char buf[1024];
    int len = snprintf(buf, sizeof(buf), "$%lu\r\n%s\r\n", strlen(s), s);
    write(fd, buf, len);
}

void handle_command(resp_object *cmd, hashtable *store, int client_fd) {
    if (cmd->type != RESP_ARRAY || cmd->elements_count == 0) {
        send_error(client_fd, "ERR invalid command format");
        return;
    }

    resp_object *first = cmd->elements[0];
    if (first->type != RESP_BULK_STRING) {
        send_error(client_fd, "ERR command must be a string");
        return;
    }

    char *command = first->str;

    if (strcasecmp(command, "PING") == 0) {
        send_simple_string(client_fd, "PONG");
    } else if (strcasecmp(command, "SET") == 0) {
        if (cmd->elements_count != 3) {
            send_error(client_fd, "ERR wrong number of arguments for 'set' command");
            return;
        }
        char *key = cmd->elements[1]->str;
        char *val = cmd->elements[2]->str;
        ht_set(store, key, val);
        send_simple_string(client_fd, "OK");
    } else if (strcasecmp(command, "GET") == 0) {
        if (cmd->elements_count != 2) {
            send_error(client_fd, "ERR wrong number of arguments for 'get' command");
            return;
        }
        char *key = cmd->elements[1]->str;
        char *val = ht_get(store, key);
        send_bulk_string(client_fd, val);
        if (val) free(val);
    } else if (strcasecmp(command, "DEL") == 0) {
        if (cmd->elements_count != 2) {
            send_error(client_fd, "ERR wrong number of arguments for 'del' command");
            return;
        }
        char *key = cmd->elements[1]->str;
        ht_del(store, key);
        // Redis DEL returns integer count of deleted keys, MVP returns OK or 1
        char buf[64];
        snprintf(buf, sizeof(buf), ":1\r\n");
        write(client_fd, buf, strlen(buf));
    } else {
        send_error(client_fd, "ERR unknown command");
    }
}