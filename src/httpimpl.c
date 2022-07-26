#define _GNU_SOURCE
#include "../include/thunks.h"
#include "../include/httpimpl.h"
#include "../include/restype.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>
#include <inttypes.h>

static result_type_of (httpmethodline_t) 
parse_methodline (raw_httpheader_t methodline)
{
  if (methodline == NULL)
    return result_with_error ("methodline is NULL");
  httpmethodline_t ret = calloc_ptr_type (httpmethodline_t);
  ret->verb = methodline;
  ret->path = strchrnul (methodline, ' ');
  if (*ret->path == '\0')
    return result_with_error ("methodline has no path");
  *ret->path++ = '\0';
  raw_httpheader_t version_hdr = strchrnul (ret->path, ' ');
  if (*version_hdr == '\0')
    return result_with_error ("methodline has no version");
  *version_hdr++ = '\0';
  if (strncmp (version_hdr, "HTTP/", 5))
    return result_with_error ("methodline has improper version format");
  raw_httpheader_t version = strchrnul (version_hdr, '.');
  if (*version == '\0')
    return result_with_error ("methodline has no version tag");
  --version;
  if (!isdigit (version[0]))
    return result_with_error ("methodline has non-numeric major version");
  if (!isdigit (version[2]))
    return result_with_error ("methodline has non-numeric minor version");
  ret->version.major = version[0] - '0';
  ret->version.minor = version[2] - '0';
  return result_with_value (ret);
}

static raw_httpheader_t
lstrip_whitespace (raw_httpheader_t header)
{
  while (isspace (*header))
    ++header;
  return header;
}

static list_t
parse_http_item_properties (raw_httpheader_t properties, char sep)
{
  cb_debug ("trying to parse: %s", properties);
  __builtin_unimplemented ();
}

static hashmap_t
parse_http_list_item (raw_httpheader_t item, char sep, char inv_sep)
{
  hashmap_t ret = g_hashmap.new ();
  ret->set (create_empty_hashmap_entry ("name"));
  ret->set (create_empty_hashmap_entry ("value"));
  ret->set (create_empty_hashmap_entry ("properties"));
  char* property_separation = strchrnul (item, inv_sep);
  if (*property_separation != '\0')
    {
      *property_separation++ = '\0';
      ret->set (create_hashmap_entry (
        "properties", parse_http_item_properties (property_separation, sep),
        false, true
      ));
    }
  char* value_separation = strchrnul (item, '=');
  if (*value_separation != '\0')
    {
      *value_separation++ = '\0';
      ret->set (create_hashmap_entry (
        "value", value_separation,
        false, false
      ));
    }
  ret->set (create_hashmap_entry (
    "name", item,
    false, false
  ));
  cb_debug ("parsed item: %s=%s", ret->get ("name"), ret->get ("value"));
  return ret;
}

static list_t
parse_http_list (raw_httpheader_t header, char sep, char inv_sep)
{
  list_t list = g_list.new ();
  while (true)
    {
      raw_httpheader_t separation = strchrnul (header, sep),
                       stripped_header = lstrip_whitespace (header);
      if (*separation == '\0')
        {
          list->append (create_list_entry (
            parse_http_list_item (stripped_header, sep, inv_sep),
            true
          ));
          break;
        }
      *separation++ = '\0';
      list->append (create_list_entry (
        parse_http_list_item (stripped_header, sep, inv_sep),
        true
      ));
      header = separation;
    }
  return list;
}

static void
identify_header_type (httpheader_t strct)
{
  raw_httpheader_t name = strct->name;
  if (is_header_equal (name, HTTPHEADER_ACCEPT))
    strct->type = HTTPHEADER_ACCEPT;
  else if (is_header_equal (name, HTTPHEADER_ACCEPT_ENCODING))
    strct->type = HTTPHEADER_ACCEPT_ENCODING;
  else if (is_header_equal (name, HTTPHEADER_COOKIE))
    strct->type = HTTPHEADER_COOKIE;
  else if (is_header_equal (name, HTTPHEADER_CONNECTION))
    strct->type = HTTPHEADER_CONNECTION;
  else if (is_header_equal (name, HTTPHEADER_USERAGENT))
    strct->type = HTTPHEADER_USERAGENT;
  else if (is_header_equal (name, HTTPHEADER_HOST))
    strct->type = HTTPHEADER_HOST;
  else if (is_header_equal (name, HTTPHEADER_KEEPALIVE))
    strct->type = HTTPHEADER_KEEPALIVE;
  else
    strct->type = HTTPHEADER_OTHER;
}

inline static void
add_to_free_list (httpcontext_t ctx, void* address)
{
  /* this is a lazy way to defer finalizing to any layer above where we
   * allocate, therefore it is up to the end user to call ctx.free() when
   * done.
   * in turn, `httpimpl.c` et al. must remember to call this function when
   * memory is allocated.
   */
  cb_debug ("adding %p to free list (size=%zu)", address,
    ctx->__int.free_list->__int.nr_entries);
  ctx->__int.free_list->append (create_list_entry (address, true));
}

static result_type_of (httpheader_t)
parse_headerline (raw_httpheader_t header)
{
  if (header == NULL)
    return result_with_error ("header is NULL");
  if (!strncmp (header, CRLF, sizeof (CRLF)))
    return result_with_value (NULL);
  httpheader_t ret = calloc_ptr_type (httpheader_t);
  raw_httpheader_t val = strchrnul (header, ':'),
                   crlf_pos = strchrnul (header, CRLF[0]);
  if (*crlf_pos == '\0')
    return result_with_error ("header is not CRLF-terminated");
  *crlf_pos = '\0';
  if (*val == '\0')
    return result_with_error ("header has no value separator");
  *val++ = '\0';
  ret->name = header;
  ret->value_as.raw = lstrip_whitespace (val);
  identify_header_type (ret);
  return result_with_value (ret);
}

static bool
parse_numeric (uintmax_t* into, raw_httpheader_t val)
{
    errno = 0;
    char *temp;
    uintmax_t res = strtoumax (val, &temp, 0);
    if (temp == val || *temp != '\0' ||
        ((res == LONG_MIN || res == LONG_MAX) && errno == ERANGE))
      return false;
    *into = res;
    return true;
}

static result_type_of (void)
update_from_header (httpcontext_t context, httpheader_t header)
{
  if (header == NULL)
    return result_with_error ("header is NULL");
  add_to_free_list (context, header);
switch (header->type)
{
case HTTPHEADER_CONNECTION:
  {
    cb_debug ("setting connection type to %s", header->value_as.raw); 
    context->connection.keep_alive.enabled
      = !strcasecmp (header->value_as.raw, "keep-alive");
    break;
  }
case HTTPHEADER_HOST:
  {
    cb_debug ("setting host to %s", header->value_as.raw); 
    context->connection.host = header->value_as.raw;
    break;
  }
case HTTPHEADER_USERAGENT:
  {
    cb_debug ("setting user agent to %s", header->value_as.raw); 
    context->connection.user_agent = header->value_as.raw;
    break;
  }
case HTTPHEADER_ACCEPT:
  {
    cb_debug ("setting accept to %s", header->value_as.raw); 
    context->connection.accept
      = parse_http_list (header->value_as.raw, ',', ';');
    break;
  }
case HTTPHEADER_KEEPALIVE:
  {
    cb_debug ("allocating keep-alive http list");
    list_t list = parse_http_list (header->value_as.raw, ',', '\0');
    list->free ();
    break;
  }
case HTTPHEADER_ACCEPT_ENCODING:
  {
    cb_debug ("allocating accept-encoding http list");
    context->connection.encoding.allowed_encodings
      = parse_http_list (header->value_as.raw, ',', ';');
    break;
  }
  default:
  {
    cb_debug (
      "adding unidentified header, name: '%s', val: '%s'",
      header->name, header->value_as.raw
    ); 
    cb_debug ("address of name: %p", header->name);
    context->connection.aux_headers->set (
      create_hashmap_entry (header->name, header->value_as.raw, false, false)
    );
    break;
  }
}
  return ok_result ();
}

static httpcontext_t
create_context (void)
{
  httpcontext_t ctx = calloc_ptr_type (typeof (ctx));
  if (ctx == NULL)
    panic ("failed to allocate space for HTTP context");
  ctx->update_from_header = g_thunks.allocate_thunk (
    "update_from_header",
    update_from_header, ctx
  );
  ctx->free = g_thunks.allocate_thunk (
    "free_context",
    free_context, ctx
  );
  ctx->__int.free_list = g_list.new ();
  ctx->connection.aux_headers = g_hashmap.new ();
  return ctx;
}

static void
free_context (httpcontext_t ctx)
{
  { /* deallocate thunks */
    cb_debug ("deallocating context thunks");
    g_thunks.deallocate_thunk (ctx->update_from_header);
    g_thunks.deallocate_thunk (ctx->free);
  }
  { /* deallocate free list */
    cb_debug ("deallocating free list and freeable addresses");
    ctx->__int.free_list->free ();
  }
  { /* free header-specific context hashmaps/lists */
#define try_free(cont) if ((cont) != NULL) cont->free ()
    try_free (ctx->cookies);
    try_free (ctx->connection.accept);
    try_free (ctx->connection.encoding.allowed_encodings);
    try_free (ctx->connection.aux_headers);
    free (ctx);
  }
}

struct __g_http_methods g_http_methods = {
  .parse_methodline = parse_methodline,
  .parse_headerline = parse_headerline,
  .create_context = create_context
};