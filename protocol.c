#include "protocol.h"

void resp_free(resp_object *obj) {
    if (!obj) return;
    if (obj->type == RESP_BULK_STRING || obj->type == RESP_SIMPLE_STRING || obj->type == RESP_ERROR) {
        free(obj->str);
    }
    if (obj->type == RESP_ARRAY) {
        for (size_t i = 0; i < obj->elements_count; i++) {
            resp_free(obj->elements[i]);
        }
        free(obj->elements);
    }
    free(obj);
}

static const uint8_t *find_crlf(const uint8_t *buf, size_t len) {
    for (size_t i = 0; i < len - 1; i++) {
        if (buf[i] == '\r' && buf[i+1] == '\n') {
            return buf + i;
        }
    }
    return NULL;
}

static long parse_long(const uint8_t *buf, size_t len) {
    char tmp[32];
    size_t copy_len = len < 31 ? len : 31;
    memcpy(tmp, buf, copy_len);
    tmp[copy_len] = '\0';
    return atol(tmp);
}

int resp_parse(const uint8_t *buf, size_t len, resp_object **out) {
    if (len == 0) return 0;

    const uint8_t *crlf = find_crlf(buf, len);
    if (!crlf && buf[0] != '$' && buf[0] != '*') return 0;

    char type = buf[0];
    resp_object *obj = calloc(1, sizeof(resp_object));
    *out = obj;

    if (type == '+') { // Simple String
        if (!crlf) { free(obj); return 0; }
        size_t str_len = crlf - (buf + 1);
        obj->type = RESP_SIMPLE_STRING;
        obj->str = malloc(str_len + 1);
        memcpy(obj->str, buf + 1, str_len);
        obj->str[str_len] = '\0';
        return (crlf + 2) - buf;
    } else if (type == '-') { // Error
        if (!crlf) { free(obj); return 0; }
        size_t str_len = crlf - (buf + 1);
        obj->type = RESP_ERROR;
        obj->str = malloc(str_len + 1);
        memcpy(obj->str, buf + 1, str_len);
        obj->str[str_len] = '\0';
        return (crlf + 2) - buf;
    } else if (type == ':') { // Integer
        if (!crlf) { free(obj); return 0; }
        obj->type = RESP_INTEGER;
        obj->integer = parse_long(buf + 1, crlf - (buf + 1));
        return (crlf + 2) - buf;
    } else if (type == '$') { // Bulk String
        if (!crlf) { free(obj); return 0; }
        long str_len = parse_long(buf + 1, crlf - (buf + 1));
        size_t header_len = (crlf + 2) - buf;
        
        if (str_len == -1) { // Null Bulk String
            obj->type = RESP_BULK_STRING;
            obj->str = NULL;
            return header_len;
        }
        
        if (len < header_len + str_len + 2) { free(obj); return 0; }
        
        obj->type = RESP_BULK_STRING;
        obj->str = malloc(str_len + 1);
        memcpy(obj->str, buf + header_len, str_len);
        obj->str[str_len] = '\0';
        obj->str_len = str_len;
        return header_len + str_len + 2;
    } else if (type == '*') { // Array
        if (!crlf) { free(obj); return 0; }
        long count = parse_long(buf + 1, crlf - (buf + 1));
        size_t bytes_consumed = (crlf + 2) - buf;
        
        obj->type = RESP_ARRAY;
        obj->elements_count = count;
        obj->elements = calloc(count, sizeof(resp_object*));
        
        for (long i = 0; i < count; i++) {
            resp_object *elem;
            int n = resp_parse(buf + bytes_consumed, len - bytes_consumed, &elem);
            if (n == 0) {
                resp_free(obj);
                *out = NULL;
                return 0;
            }
            if (n < 0) {
                resp_free(obj);
                *out = NULL;
                return -1;
            }
            obj->elements[i] = elem;
            bytes_consumed += n;
        }
        return bytes_consumed;
    }
    
    free(obj);
    *out = NULL;
    return -1; // Unknown type
}