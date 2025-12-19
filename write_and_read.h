#ifndef NET_UTILS_H
#define NET_UTILS_H

#include <stdint.h>  // for int32_t
#include <vector>
#include "conn.h"
#include "buff.h"

/**
 * @brief Reads exactly 'length' bytes from a connected socket.
 *
 * This function repeatedly calls recv() until the specified number of bytes
 * have been read or an error/EOF occurs.
 *
 * @param connfd  The connected socket file descriptor.
 * @param rbuf    Pointer to the buffer where the received data will be stored.
 * @param length  Number of bytes to read.
 * @return 0 on success, -1 on error or EOF.
 */
int32_t read_full(int connfd, char *rbuf, int length);

/**
 * @brief Writes exactly 'length' bytes to a connected socket.
 *
 * This function repeatedly calls send() until all bytes in the buffer
 * are written to the socket or an error/EOF occurs.
 *
 * @param connfd  The connected socket file descriptor.
 * @param rbuf    Pointer to the buffer containing the data to send.
 * @param length  Number of bytes to write.
 * @return 0 on success, -1 on error or EOF.
 */
int32_t write_all(int connfd, char *rbuf, int length);

/**
 * @brief Prints an error message using perror().
 *
 * This helper function is useful for printing system error messages.
 *
 * @param message  The message to print before the system error description.
 */
void msg(const char *message);

void die(const char* message);

void fd_set_nb(int fd);

bool try_one_request(Conn *conn);

void buf_append(Buffer *buf, const uint8_t *data, size_t len);

void buf_consume(Buffer *buf, size_t len);

#endif // NET_UTILS_H
