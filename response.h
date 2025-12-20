#ifndef RESPONSE_H
#define RESPONSE_H

#include <cstdint>
#include <vector>

struct Response {
    uint32_t status = 0;
    std::vector<uint8_t> data;
};



#endif
