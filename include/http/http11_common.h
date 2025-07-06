#pragma once

#include <sys/types.h>
#include <cerrno>

#define check(A, M, ...) if(!(A)) { /*log_err(M, ##__VA_ARGS__);*/ errno=0; goto error; }

typedef void (*element_cb)(void *data, const char *at, size_t length);
typedef void (*field_cb)(void *data, const char *field, size_t flen, const char *value, size_t vlen);