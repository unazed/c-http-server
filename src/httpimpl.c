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

static httphashlist_t
parse_http_item_properties (raw_httpheader_t properties, char sep)
{
  __builtin_unimplemented ();
}

static httphashlist_t
parse_http_list_item (raw_httpheader_t item, char sep, char inv_sep)
{
  httphashlist_t ret = g_hashmap.new ();
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
  return ret;
}

static httphashlist_t
parse_http_list (raw_httpheader_t header, char sep, char inv_sep)
{
  httphashlist_t hash_list = g_hashmap.new ();
  while (true)
    {
      raw_httpheader_t separation = strchrnul (header, sep),
                       stripped_header = lstrip_whitespace (header);
      if (*separation == '\0')
        {
          hash_list->set (create_hashmap_entry (
            stripped_header,
            parse_http_list_item (stripped_header, sep, inv_sep),
            false, true
          ));
          break;
        }
      *separation++ = '\0';
      hash_list->set (create_hashmap_entry (
        stripped_header,
        parse_http_list_item (stripped_header, sep, inv_sep),
        false, true
      ));
      header = separation;
    }
  return hash_list;
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

static void
add_to_free_list (httpcontext_t ctx, void* address)
{
  /* this is a lazy way to defer finalizing to any layer above where we
   * allocate, therefore it is up to the end user to call ctx.free() when
   * done.
   * in turn, `httpimpl.c` et al. must remember to call this function when
   * memory is allocated.
   */
  cb_debug ("adding %p to free list (size=%zu)", address,
    ctx->__int_free_list.nr_addresses);
  if (!ctx->__int_free_list.nr_addresses)
    {
      ctx->__int_free_list.addresses
        = calloc (1, sizeof (*ctx->__int_free_list.addresses));
      ctx->__int_free_list.nr_addresses++;
    }
  else
    ctx->__int_free_list.addresses = realloc (
      ctx->__int_free_list.addresses,
      sizeof (*ctx->__int_free_list.addresses) *
      ++ctx->__int_free_list.nr_addresses
    );
  if (ctx->__int_free_list.addresses == NULL)
    panic ("failed to allocate space for context freelist");
  ctx->__int_free_list.addresses[ctx->__int_free_list.nr_addresses - 1]
    = address;
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
    httphashlist_t list = parse_http_list (header->value_as.raw, ',', '\0');
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
    cb_debug ("parsing unknown header, name: '%s', val: '%s'",
        header->name, header->value_as.raw); 
    break;
  }
}
  return ok_result ();
}

static httpcontext_t
create_context (void)
{
  httpcontext_t ctx = calloc (1, sizeof (*ctx));
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
  return ctx;
}

static void
free_context (httpcontext_t ctx)
{
  cb_debug ("deallocating context thunks");
  g_thunks.deallocate_thunk (ctx->update_from_header);
  g_thunks.deallocate_thunk (ctx->free);
  for (size_t i = 0; i < ctx->__int_free_list.nr_addresses; ++i)
    {
      cb_debug ("deallocating context free list item %zu", i);
      free (ctx->__int_free_list.addresses[i]);
    }
#define try_free(map) if ((map) != NULL) map->free ()
  try_free (ctx->cookies);
  try_free (ctx->connection.accept);
  try_free (ctx->connection.encoding.allowed_encodings);
  free (ctx);
}

struct __g_http_methods g_http_methods = {
  .parse_methodline = parse_methodline,
  .parse_headerline = parse_headerline,
  .create_context = create_context
};