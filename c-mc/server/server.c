#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <string.h>
#include "hash.c"
#include "utils.h"
#include <sys/fcntl.h>
#include <sys/socket.h>
#include <sys/event.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

#define PORT 8080

typedef struct Conn {
    int fd;
    char *wbuf; // pending write buffer
    size_t wlen; // bytes valid in wbuf
    size_t woff; // next byte to write
} Conn;

void handle_client(int client_fd) {
    uint8_t buf[MSG_WIRE_SIZE];
    size_t got = 0;

    while (got < MSG_WIRE_SIZE) {
        ssize_t n = recv(client_fd, buf + got, MSG_WIRE_SIZE - got, 0);
        if (n < 0) {
            if (errno == EINTR) continue;
            perror("recv");
            break;
        }
        if (n == 0) {
            break;
        }
        got += (size_t)n;
    }
    
    if (got == MSG_WIRE_SIZE) {
        mc_task_t m;
        deserialize_msg(&m, buf);
        printf("trials: %llu result: %f\n", m.trials, m.result);
    }
}

static int set_nonblock(int fd) {
    int fl = fcntl(fd, F_GETFL, 0);
    if (fl == -1) return -1;
    return fcntl(fd, F_SETFL, fl | O_NONBLOCK);
}

static int create_listener(uint16_t port) {
    int server_fd;
    int opt = 1;
    struct sockaddr_in server_addr;

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket error");
        return -1;
    }
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt));

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(8080);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind error");
        return -1;
    }

    if (listen(server_fd, SOMAXCONN) < 0) {
        perror("listen error");
        return -1;
    }

    return server_fd;
}

static void ev_add(int kq, int fd, int filt, void *udata) {
    struct kevent ev;
    EV_SET(&ev, fd, filt, EV_ADD | EV_ENABLE, 0, 0, udata);
    if (kevent(kq, &ev, 1, NULL, 0, NULL) < 0) perror("kevent add");
}

static void ev_del(int kq, int fd, int filt) {
    struct kevent ev;
    EV_SET(&ev, fd, filt, EV_DELETE, 0, 0, NULL);
    kevent(kq, &ev, 1, NULL, 0, NULL);
}

static void close_conn(int kq, Conn *c) {
    if (!c) return;
    ev_del(kq, c->fd, EVFILT_READ);
    ev_del(kq, c->fd, EVFILT_WRITE);
    close(c->fd);
    free(c->wbuf);
    free(c);
}

int main() {
    ht* table = createHashTable(TABLE_SIZE);
    int sfd = create_listener(PORT);
    printf("Server running on port: %d\n", PORT);

    for (;;) {
        struct sockaddr_in client_addr;
        socklen_t len = sizeof(client_addr);
        int client_fd = accept(sfd, (struct sockaddr*)&client_addr, &len);
        //set_nonblock(client_fd);
        insert(table, client_fd);
        handle_client(client_fd);
    }

    free(table);
    close(sfd);
    return 0;
}