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
#include <fcntl.h>      // for fcntl(), F_SETFL, F_GETFL, O_NONBLOCK
#include "conn.h"
#include <cstdint>     // for uint8_t
#include "buff.h"
#include <vector>
#include <string>
#include "response.h"
#include <map>
#include "data.h"

const size_t K_MAX_MSG = 4096;
// placeholder; implemented later

static bool read_u32(const uint8_t *&cur, const uint8_t *end, uint32_t &out) {

    if (cur + 4 > end) return false;

    memcpy(&out, cur, 4);
    cur += 4;
    return true;
}

static void out_err(Buffer &out, uint8_t err, std::string msg) {

    const uint32_t size = 1 + msg.size();
    buf_append(&out, (uint8_t *)&size, 4);
    buf_append(&out, &err, 1);
    buf_append(&out, (uint8_t *) msg.data(), msg.size());
}

// reserve 4 bytes for response header 
static void response_begin(Buffer &out, size_t *header) {
    uint32_t z = 0;
    *header = out.size(); // so we know where to put the header lmfao
    buf_append(&out, (uint8_t * ) &z, 4);
}

static size_t response_len(Buffer &out, size_t header) {

    return out.size() - header - 4;
}

static void response_end(Buffer &out, size_t header) {

    size_t s = response_len(out, header);

    if (s > K_MAX_MSG) {
        out.data_end = out.data_begin + header + 4; // some room for the header would be nice
        out_err(out, 1, "msg too big");
    }
    // FINISH DIS TMRW
    ;
}
static bool read_str(const uint8_t *&cur, const uint8_t *end, size_t n, std::string &out) {

    if (cur + n > end) return false;

    out.assign(reinterpret_cast<const char*>(cur), n);
    cur += n;

    return true;

}

int32_t parse_req(const uint8_t *data, size_t size, std::vector<std::string> &cmd) {

    const uint8_t *cur = data;
    const uint8_t *end = data + size;

    uint32_t nstr = 0;
    if (!read_u32(cur, end, nstr)) return -1;

    cmd.clear();
    cmd.reserve(nstr);

    while (cmd.size() < nstr) {
        uint32_t len = 0;
        if (!read_u32(cur, end, len)) return -1;

        cmd.emplace_back();
        if (!read_str(cur, end, len, cmd.back())) return -1;
    }

    if (cur != end) return -1; // trailing garbage
    return 0;

}

void do_request(std::vector<std::string> &cmd, Response& out, Buffer& outgoing) {

    if (cmd.size() == 2 && cmd[0] == "get") {
        do_get(cmd, outgoing);

    } else if (cmd.size() == 3 && cmd[0] == "set") {

        do_set(cmd, outgoing);

    } else if (cmd.size() == 2 && cmd[0] == "del") {
        do_delete(cmd, outgoing);

    } else {
    
        out.status = 2;
        uint32_t &status = out.status;
        uint32_t resp_len = 4;
        buf_append(&outgoing, reinterpret_cast<uint8_t*>(&resp_len), 4);
        buf_append(&outgoing, reinterpret_cast<uint8_t*>(&status), 4);   // unrecognized command
    }

}

void make_response(Response& resp, Buffer& out) {
    // need to serialize
    uint32_t resp_len = 4 + (uint32_t) resp.data.size();
    buf_append(&out, (const uint8_t *)&resp_len, 4);
    buf_append(&out, (const uint8_t *)&resp.status, 4);
    buf_append(&out, resp.data.data(), resp.data.size());
}

int32_t read_full(int connfd, char *rbuf, int length) {

    while (length > 0) {
        ssize_t rv = recv(connfd, rbuf, length , 0);

        if (rv <= 0) {
            return -1; // error or EOF
        }

        assert((size_t) rv <= length);

        length -= (size_t) rv;
        rbuf += (size_t) rv;
    }

    return 0;
}

int32_t write_all(int connfd, char *rbuf, int length) {

    // write everything from buff into the connfd byte stream

    while (length > 0 ) {

        ssize_t rv = send(connfd, rbuf, length, 0);

        if (rv <= 0) {
            return -1; // error or EOF
        }

        assert((size_t) rv <= length);

        length -= (size_t) rv;
        rbuf += rv;
    }

    return 0;
}

void msg(const char* message) {
    perror(message);
}

void die(const char* message) { 

    perror(message); exit(1);

}

void fd_set_nb(int fd) {
    fcntl(fd, F_SETFL, fcntl(fd, F_GETFL, 0) | O_NONBLOCK);
}

int buf_append(struct Buffer *buf, const uint8_t *data, size_t len) {
    /**
     * Tries to append to the buffer, returns -1 if the buffer is full
     * @returns true if sucessful, false if non-successful
     */

    if (buf->data_end + len > buf->buffer_end) {return -1;}
    memcpy(buf->data_end, data, len);
    buf->data_end += len;
    return 0;
}

void buf_consume(struct Buffer *buf, size_t len) {
    buf->data_begin += len;

    if (buf->data_begin == buf->data_end) {
        buf->data_begin = buf->buffer_begin;
    }

    return;
}

bool try_one_request(Conn *conn) {
    /**
     * Tries to parse the one request within conn incoming, and will parse the command as well as trying to create the response data.
     * @returns true if sucessful, false if non-successful
     */

    if (conn->incoming.size() < 4){
        return false;
    }
    
    uint32_t be_len = 0;
    memcpy(&be_len, conn->incoming.data(), 4);
    uint32_t length = ntohl(be_len);

    if (length > K_MAX_MSG) {
        conn->want_close = true; // fairly idiomatic what i'm doing here
        return false;
    }

    if (conn->incoming.size() < 4 + length) return false;

    std::vector<std::string> cmd;

    if (parse_req(conn->incoming.data() + 4, length, cmd) < 0) {
        conn->want_close = true;
        return false;
    }
    
    Response resp; // we may need it later
    do_request(cmd, resp, conn->outgoing);

    // 5. Remove the message from `Conn::incoming`.
    buf_consume(&conn->incoming, 4 + length);

    return true;
} 

