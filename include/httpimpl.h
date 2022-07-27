#ifndef __HTTPIMPL_H
#define __HTTPIMPL_H

#include "restype.h"
#include <stdint.h>

typedef char* raw_httpheader_t;

typedef struct
{
  raw_httpheader_t verb;
  raw_httpheader_t path;
  struct {
    uint8_t minor; uint8_t major;
  } version;
} *httpmethodline_t;

result_type_of (httpmethodline_t) 
parse_methodline (raw_httpheader_t methodline);

struct __g_http_methods
{
  typeof (parse_methodline)* parse_methodline;
};

extern struct __g_http_methods g_http_methods;

#endif /* __HTTPIMPL_H */