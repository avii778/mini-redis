#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <assert.h>
#include "write_and_read.h"

const size_t K_MAX_MSG = 4096;

void die(const char* message) { 

    perror(message);
    exit(1);

}

static int32_t query(int fd, const char* text){

    int32_t length = strlen(text);

    if (length > K_MAX_MSG) {
        perror("Message too long");
        return -1;
    }
    // write in our header and other shit
    uint32_t be = htonl(length);
    char buf[4 + length];
    memcpy(buf, &be, 4);
    memcpy(&buf[4], text, length);
    int err = write_all(fd, buf, length + 4);

    if (err) {
        return err;
    }

    char rbuf[4 + K_MAX_MSG];
    errno = 0;

    err = read_full(fd, rbuf, 4); // read len of response
    if (err) {
        msg(errno == 0 ? "EOF" : "read() error");
        return err;
    }

    memcpy(&length, rbuf, 4); // please be little endian
    length = ntohl(length);
    if (length > K_MAX_MSG) {
        msg("too long");
        return -1;
    }

    err = read_full(fd, &rbuf[4], length);

    if (err) {
        msg("read() error");
        return err;
    }

    // do something
    printf("server says: %.*s\n", length, &rbuf[4]);
    return 0;
}

int main() {

    int fd = socket(AF_INET, SOCK_STREAM, 0);

    if (fd < 0) {
        die("socket()");
    }

    struct sockaddr_in addr = {}; 
    addr.sin_family = AF_INET;
    addr.sin_port = htons(1234);
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

    int rv = connect(fd, (const sockaddr *)&addr, sizeof(addr));

    if (rv) {die("Connect error");};

    char wbuf[] = "please play with my ween";

    rv = query(fd, wbuf);

    if (rv) {
        goto L_DONE;
    }

    L_DONE:
        close(fd);
        return 0;
    return 0;
}
