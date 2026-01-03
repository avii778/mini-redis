#ifndef SERIALIZATION_H
#define SERIALIZATION_H

#include <stdint.h>

enum Tag : uint8_t {

    TAG_NIL = 0,    // nil
    TAG_ERR = 1,    // error code + msg
    TAG_STR = 2,    // string
    TAG_INT = 3,    // int64
    TAG_DBL = 4,    // double
    TAG_ARR = 5,    // array

};

#endif 