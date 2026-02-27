#ifndef PROTOCOL_H
#define PROTOCOL_H

#include "common.h"

typedef enum {
    RESP_SIMPLE_STRING,
    RESP_ERROR,
    RESP_INTEGER,
    RESP_BULK_STRING,
    RESP_ARRAY
} resp_type;

typedef struct resp_object {
    resp_type type;
    long integer;
    char *str;
    size_t str_len;
    struct resp_object **elements;
    size_t elements_count;
} resp_object;

void resp_free(resp_object *obj);
int resp_parse(const uint8_t *buf, size_t len, resp_object **out);

#endif