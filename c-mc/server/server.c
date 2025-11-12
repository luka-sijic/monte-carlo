#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <string.h>
#include <sys/_types/_socklen_t.h>
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
    printf("Client");
}

static int set_nonblock(int fd) {
    int fl = fcntl(fd, F_GETFL, 0);
    if (fl == -1) return -1;
    return fcntl(fd, F_SETFL, fl | O_NONBLOCK);
}

static int create_listener(uint16_t port) {
    printf("Server running on port: %d", PORT);
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
    int sfd = create_listener(PORT);

    struct kevent event;

    int kq = kqueue();

    ev_add(kq, sfd, EVFILT_READ, NULL);
    for (;;) {
        struct kevent evs[128];
        int n = kevent(kq, NULL, 0, evs, (int)(sizeof evs / sizeof *evs), NULL);
        if (n < 0) { if (errno == EINTR) continue; perror("kevent wait"); break; }

        for (int i = 0;i < n;i++) {
            int fd = evs[i].ident;

            // Out loop iterates over ready kqueue events
            // If ready FD is the listener, accept all pending clients now
            // For each client: set non-blocking, subscribe to EVFILT_READ, and log
            if (fd == sfd) {
                for (;;) {
                    struct sockaddr_in cli; socklen_t len = sizeof cli;
                    int cfd = accept(sfd, (struct sockaddr*)&cli, &len);
                    if (cfd < 0) {
                        if (errno == EAGAIN || errno == EWOULDBLOCK) break;
                        perror("accept"); break;
                    }
                    set_nonblock(cfd);
                    Conn *c = calloc(1, sizeof *c);
                    c->fd = cfd;
                    ev_add(kq, cfd, EVFILT_READ, c);
                    char ip[64]; inet_ntop(AF_INET, &cli.sin_addr, ip, sizeof ip);
                    printf("client %s:%d fd=%d\n", ip, ntohs(cli.sin_port), cfd);
                }
                continue;
            }

            Conn *c = (Conn*)evs[i].udata;

            if (evs[i].filter == EVFILT_READ) {
                if (evs[i].flags & EV_EOF) {
                    close_conn(kq, c);
                    continue;
                }
                char buf[4096];
                for (;;) {
                    ssize_t r = read(fd, buf, sizeof buf);
                    if (r > 0) {
                        if (c->wlen > c->woff) {
                            size_t old_pending = c->wlen - c->woff;
                            char *nbuf = realloc(c->wbuf, old_pending + (size_t)r);
                            if (!nbuf) { close_conn(kq, c); break; }
                            memcpy(nbuf + old_pending, buf, (size_t)r);
                            c->wbuf = nbuf;
                            c->wlen = old_pending + (size_t)r;
                            c->woff = 0;
                            ev_add(kq, fd, EVFILT_WRITE, c);
                        } else {
                            size_t off = 0;
                            while (off < (size_t)r) {
                                ssize_t w = write(fd, buf + off, (size_t)r - off);
                                if (w > 0) { off += (size_t)w; continue; }
                                if (w < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
                                    // buffer the remainder and watch for writable
                                    c->wbuf = malloc((size_t)r - off);
                                    if (!c->wbuf) { close_conn(kq, c); goto next_event; }
                                    memcpy(c->wbuf, buf + off, (size_t)r - off);
                                    c->woff = 0;
                                    c->wlen = (size_t)r - off;
                                    ev_add(kq, fd, EVFILT_WRITE, c);
                                    break;
                                }
                                if (w < 0 && errno == EINTR) continue;
                                // fatal error
                                close_conn(kq, c);
                                goto next_event;
                            }
                        }
                        // keep draining read readiness
                        continue;
                    }
                    if (r == 0) { // EOF
                        close_conn(kq, c);
                    } else if (r < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
                        // no more to read
                    } else if (r < 0 && errno == EINTR) {
                        continue;
                    } else if (r < 0) {
                        perror("read");
                        close_conn(kq, c);
                    }
                    break;
                }
            } else if (evs[i].filter == EVFILT_WRITE) {
                // flush pending buffer
                while (c->woff < c->wlen) {
                    ssize_t w = write(fd, c->wbuf + c->woff, c->wlen - c->woff);
                    if (w > 0) { c->woff += (size_t)w; continue; }
                    if (w < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) break;
                    if (w < 0 && errno == EINTR) continue;
                    // fatal
                    close_conn(kq, c);
                    goto next_event;
                }
                if (c && c->woff >= c->wlen) {
                    free(c->wbuf); c->wbuf = NULL; c->wlen = c->woff = 0;
                    ev_del(kq, fd, EVFILT_WRITE); 
                }
            }
        next_event:
            ;
        }
    }

    close(sfd);
    close(kq);
    return 0;
}