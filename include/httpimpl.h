#ifndef __HTTPIMPL_H
#define __HTTPIMPL_H

#include <stdint.h>
#include <time.h>
#include <stdbool.h>
#include "restype.h"

typedef char* raw_httpheader_t;
typedef time_t httptimeval_t;
typedef bool httpbool_t;

typedef struct
{
  raw_httpheader_t name;
  raw_httpheader_t value;
  struct attr
  {
    httptimeval_t expires, max_age;
    raw_httpheader_t domain, path, same_site;
    bool secure, http_only;
  };
} *httpcookiejar_t;

struct http_list_item
{
  raw_httpheader_t name;
  raw_httpheader_t value;
};

typedef struct
{
  struct http_list_item* items;
  size_t nr_items;
} *httplist_t;


enum httpheader_value_type
{
  HTTPHEADER_ACCEPT,
  HTTPHEADER_ACCEPT_ENCODING,
  HTTPHEADER_COOKIE,
  HTTPHEADER_OTHER
};

typedef struct
{
  enum httpheader_value_type type;
  raw_httpheader_t name;
  union value_as
  {
    httpcookiejar_t cookiejar;
    raw_httpheader_t raw;
    httplist_t list;
  };
} *httpheader_t;

typedef struct
{
  raw_httpheader_t verb;
  raw_httpheader_t path;
  struct {
    uint8_t minor; uint8_t major;
  } version;
} *httpmethodline_t;

static result_type_of (httpmethodline_t) 
parse_methodline (raw_httpheader_t methodline);

static result_type_of (httpheader_t)
parse_headerline (raw_httpheader_t header);

struct __g_http_methods
{
  typeof (parse_methodline)* parse_methodline;
  typeof (parse_headerline)* parse_headerline;
};

extern struct __g_http_methods g_http_methods;

#endif /* __HTTPIMPL_H */