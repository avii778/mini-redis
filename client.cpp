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
#include <string>
#include <vector>

const size_t K_MAX_MSG = 4096;

static int32_t send_req(int fd, const char *text, size_t len) {
    uint32_t length = (uint32_t)len;
    if (length > K_MAX_MSG) {
        perror("Message too long");
        return -1;
    }

    uint32_t be = htonl(length);
    char buf[4 + K_MAX_MSG];
    memcpy(buf, &be, 4);
    memcpy(&buf[4], text, len);
    int err = write_all(fd, buf, length + 4);
    if (err) {
        return err;
    }
    return 0;
}

static int32_t read_res(int fd) {
    char rbuf[4 + K_MAX_MSG];
    errno = 0;
    int32_t err = read_full(fd, rbuf, 4);
    if (err) {
        msg(errno == 0 ? "EOF" : "read() error");
        return err;
    }

    uint32_t length = 0;
    memcpy(&length, rbuf, 4);
    if (length > K_MAX_MSG) {
        msg("too long");
        return -1;
    }

    err = read_full(fd, &rbuf[4], length); // read the response body
    if (err) {
        msg("read() error");
        return err;
    }

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

    std::vector<std::string> query_list = {
        "hello", "world", "pipelining", "test"
    };

    for (const std::string &s : query_list) {
        int32_t err = send_req(fd, s.data(), s.size());
        if (err) {
            goto L_DONE;
        }
    }

    for (size_t i = 0; i < query_list.size(); ++i) {
        int32_t err = read_res(fd);
        if (err) {
            goto L_DONE;
        }
    }

    L_DONE:
        close(fd);
        return 0;
}
