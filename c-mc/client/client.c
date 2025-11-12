#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>

#define PORT 8080

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

int main() {
    int cfd;
    struct sockaddr_in;

    cfd = socket(AF_INET, SOCK_STREAM, 0);
}