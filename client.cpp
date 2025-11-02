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

int main() {

    int fd = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in addr = {}; 
    addr.sin_family = AF_INET;
    addr.sin_port = htons(1234);
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

    char wbuf[] = "Hello";

    int rv = connect(fd, (const sockaddr *)&addr, sizeof(addr));

    if (rv) {die("Connect error");};

    rv = send(fd, wbuf, strlen(wbuf), 0);

    char rbuf[64] = {};

    ssize_t n = recv(fd, rbuf, sizeof(rbuf) - 1, 0);

    if (n < 0) {
        die("read");
    }
    printf("server says: %s\n", rbuf);
    close(fd);


    return 0;

}
