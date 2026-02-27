#include "server.h"
#include "common.h"
#include "protocol.h"
#include "hashtable.h"
#include "commands.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <sys/epoll.h>

#define MAX_EVENTS 64
#define BACKLOG 128
#define READ_BUF_SIZE 4096

static hashtable *store;

void set_nonblocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

void server_start(int port) {
    int server_fd, epoll_fd;
    struct sockaddr_in addr;
    struct epoll_event ev, events[MAX_EVENTS];

    store = ht_init(1024);

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        LOG_ERR("Socket creation failed");
        exit(1);
    }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    if (bind(server_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        LOG_ERR("Bind failed");
        exit(1);
    }

    if (listen(server_fd, BACKLOG) < 0) {
        LOG_ERR("Listen failed");
        exit(1);
    }

    set_nonblocking(server_fd);

    epoll_fd = epoll_create1(0);
    if (epoll_fd < 0) {
        LOG_ERR("Epoll creation failed");
        exit(1);
    }

    ev.events = EPOLLIN;
    ev.data.fd = server_fd;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fd, &ev) < 0) {
        LOG_ERR("Epoll add failed");
        exit(1);
    }

    LOG_INFO("Server listening on port %d...", port);

    while (1) {
        int nfds = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
        for (int i = 0; i < nfds; i++) {
            if (events[i].data.fd == server_fd) {
                struct sockaddr_in client_addr;
                socklen_t client_len = sizeof(client_addr);
                int client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_len);
                if (client_fd < 0) continue;
                
                set_nonblocking(client_fd);
                ev.events = EPOLLIN | EPOLLET;
                ev.data.fd = client_fd;
                epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &ev);
                LOG_INFO("New connection");
            } else {
                int fd = events[i].data.fd;
                uint8_t buf[READ_BUF_SIZE];
                ssize_t n = read(fd, buf, sizeof(buf));
                
                if (n <= 0) {
                    close(fd);
                    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, NULL);
                } else {
                    resp_object *cmd = NULL;
                    int parsed = resp_parse(buf, n, &cmd);
                    
                    if (parsed > 0 && cmd) {
                        handle_command(cmd, store, fd);
                        resp_free(cmd);
                    } else if (parsed == 0) {
                        // Incomplete, buffer handling needed for V2
                    } else {
                        write(fd, "-ERR parsing error\r\n", 20);
                    }
                }
            }
        }
    }
}