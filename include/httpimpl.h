#ifndef __HTTPIMPL_H
#define __HTTPIMPL_H

#include <stdint.h>
#include <time.h>
#include <stdbool.h>
#include "restype.h"
#include "hashmap.h"
#define CRLF ("\r\n")

typedef char* raw_httpheader_t;
typedef time_t httptimeval_t;
typedef bool httpbool_t;

typedef hashmap_t httpcookiejar_t;
typedef hashmap_t httphashlist_t;

enum httpheader_value_type
{
  HTTPHEADER_ACCEPT = 0,
  HTTPHEADER_ACCEPT_ENCODING,
  HTTPHEADER_COOKIE,
  HTTPHEADER_CONNECTION,
  HTTPHEADER_KEEPALIVE,
  HTTPHEADER_USERAGENT,
  HTTPHEADER_HOST,
  HTTPHEADER_OTHER,
  HTTPHEADER_INVALID
};

#define header_name(ty) httpheader_value_names[(ty)]
#define is_header_equal(a, ty) (!strcasecmp ((a), (header_name(ty))))
static const char* const httpheader_value_names[] = {
  [HTTPHEADER_ACCEPT] = "accept",
  [HTTPHEADER_ACCEPT_ENCODING] = "accept-encoding",
  [HTTPHEADER_COOKIE] = "cookie",
  [HTTPHEADER_CONNECTION] = "connection",
  [HTTPHEADER_KEEPALIVE] = "keep-alive",
  [HTTPHEADER_USERAGENT] = "user-agent",
  [HTTPHEADER_HOST] = "host"
}; /* if adding additional methods, update the enum and
    * `identify_header_type` in `src/httpimpl.c` accordingly
    */

typedef struct
{
  enum httpheader_value_type type;
  raw_httpheader_t name;
  union
  {
    httpcookiejar_t cookiejar;
    raw_httpheader_t raw;
    httphashlist_t hash_list;
  } value_as;
} *httpheader_t;

__THUNK_DECL static result_type_of (void)
update_from_header_thunk (httpheader_t header);

__THUNK_DECL static void
free_context_thunk (void);

typedef const char* (*generic_compression_fn)(const char* plaintext);
typedef const char* (*generic_decompression_fn)(const char* compressed);

struct encoding_type
{
  generic_compression_fn compress;
  generic_decompression_fn decompress;
  void* reserved;
};

typedef struct
{
  struct 
  {
    struct 
    {
      httphashlist_t allowed_encodings;
      struct encoding_type* chosen_encoding;
    } encoding;
    struct 
    {
      bool enabled;
      httptimeval_t timeout;
      size_t max_reqs;  /* not enforced */
    } keep_alive;
    httphashlist_t accept;
    raw_httpheader_t user_agent;
    raw_httpheader_t host;
  } connection;
  struct
  {
    void ** addresses;
    size_t nr_addresses;
  } __int_free_list;
  httpcookiejar_t cookies;
  typeof (free_context_thunk)* free;
  typeof (update_from_header_thunk)* update_from_header;
} *httpcontext_t;

static result_type_of (void)
update_from_header (httpcontext_t this, httpheader_t header);

static void
free_context (httpcontext_t ctx);

typedef struct
{
  raw_httpheader_t verb;
  raw_httpheader_t path;
  struct
  {
    uint8_t minor; uint8_t major;
  } version;
} *httpmethodline_t;

static result_type_of (httpmethodline_t) 
parse_methodline (raw_httpheader_t methodline);

static result_type_of (httpheader_t)
parse_headerline (raw_httpheader_t header);

static httpcontext_t
create_context (void);

struct __g_http_methods
{
  typeof (parse_methodline)* parse_methodline;
  typeof (parse_headerline)* parse_headerline;
  typeof (create_context)* create_context;
};

extern struct __g_http_methods g_http_methods;

#endif /* __HTTPIMPL_H */