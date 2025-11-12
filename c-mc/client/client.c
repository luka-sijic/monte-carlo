// client.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

int main(void) {
    const char *server_ip = "127.0.0.1";
    const uint16_t port = 8080;

    int cfd = socket(AF_INET, SOCK_STREAM, 0);
    if (cfd < 0) { perror("socket"); exit(EXIT_FAILURE); }

    struct sockaddr_in srv;
    memset(&srv, 0, sizeof(srv));
    srv.sin_family = AF_INET;
    srv.sin_port   = htons(port);
    if (inet_pton(AF_INET, server_ip, &srv.sin_addr) != 1) {
        perror("inet_pton");
        close(cfd);
        exit(EXIT_FAILURE);
    }

    if (connect(cfd, (struct sockaddr*)&srv, sizeof(srv)) < 0) {
        perror("connect");
        close(cfd);
        exit(EXIT_FAILURE);
    }

    const char *msg = "hello, server\n";
    if (send(cfd, msg, strlen(msg), 0) < 0) { perror("send"); close(cfd); exit(EXIT_FAILURE); }

    char buf[4096];
    ssize_t n = recv(cfd, buf, sizeof(buf)-1, 0);
    if (n < 0) { perror("recv"); close(cfd); exit(EXIT_FAILURE); }
    buf[n] = '\0';
    printf("Received: %s", buf);

    close(cfd);
    return 0;
}
