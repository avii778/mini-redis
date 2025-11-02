#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void die(const char* message) { 

    perror(message);
    exit(1);

}

void do_something(const int connfd) {

    char rbuf[64] = {};
    ssize_t rv = recv(connfd, rbuf, sizeof(rbuf) - 1, 0);

    if (rv <= 0) {
        die("recv() error");
        return;
    }

    printf("Client says %s\n", rbuf);

    char wbuf[] = "world";

    rv = send(connfd, wbuf, strlen(wbuf), 0); 

    if (rv == -1){
        die("Send() error");
    } 

    close(connfd);

    return;
}

int main() {

    int fd = socket(AF_INET, SOCK_STREAM, 0);

    int val = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));
    struct sockaddr_in addr = {};

    addr.sin_family = AF_INET;
    addr.sin_port = htons(1234);
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

    int rv = bind(fd, (const struct sockaddr *)&addr, sizeof(addr));
    if (rv) { die("bind()"); }

    rv = listen(fd, SOMAXCONN);

    while (true) {

        struct sockaddr_in client_addr = {};
        socklen_t addr_len = sizeof(client_addr);

        int connfd = accept(fd, (struct sockaddr *)&client_addr, &addr_len);

        if (connfd < 0) {
            continue;   // error
        }

        do_something(connfd);


    }


    return 0;

}
