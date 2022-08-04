#define _GNU_SOURCE
#include "../include/thunks.h"
#include "../include/httpimpl.h"
#include "../include/restype.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>
#include <inttypes.h>

#define try_alloc_space_for(ptr_ty) \
  calloc (1, sizeof (*(ptr_ty)0)); \
  if (ret == NULL) \
    return result_with_error ("failed to allocate space for " #ptr_ty)

static result_type_of (httpmethodline_t) 
parse_methodline (raw_httpheader_t methodline)
{
  if (methodline == NULL)
    return result_with_error ("methodline is NULL");
  httpmethodline_t ret = try_alloc_space_for (httpmethodline_t);
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
strip_whitespace (const raw_httpheader_t header)
{
  if (header == NULL)
    return NULL;
  raw_httpheader_t ret = header;
  while (isspace (*ret))
    ++ret;
  return ret;
}

static struct http_list_item*
parse_http_list_item (raw_httpheader_t entry, char inv_sep)
{
  struct http_list_item* item = calloc (1, sizeof (*item));
  raw_httpheader_t sep = strchrnul (entry, '=');
  if (*sep == '\0')
    {
      /* e.g.: `Accept: text/html,application/xhtml+xml */
      item->name = strip_whitespace (entry);
      return item;
    }
  *sep++ = '\0';
  item->name = strip_whitespace (entry);
  item->value = strip_whitespace (sep);
  return item;
}

static httplist_t
parse_http_list (raw_httpheader_t header, char sep, char inv_sep)
{
  httplist_t list = calloc (1, sizeof (*list));
  if (list == NULL)
    panic ("failed to allocate HTTP list");

  list->items = calloc (1, sizeof (*list->items));
  if (list->items == NULL)
    panic ("failed to allocate HTTP list items");

  while (*header)
    {
      raw_httpheader_t next_hdr = strchrnul (header, sep);
      if (*next_hdr == '\0')
        {
          /* when on last item */
          list->items[list->nr_items++]
            = parse_http_list_item (header, inv_sep);
          break;
        }
      *next_hdr++ = '\0';
      list->items[list->nr_items++] = parse_http_list_item (header, inv_sep);
      list->items = realloc (list->items,
        sizeof (*list->items) * (list->nr_items + 1));
      if (list->items == NULL)
        panic ("failed to allocate (further) HTTP list items");
      header = next_hdr;
    }
  cb_debug ("parsed HTTP list with %zu items", list->nr_items);
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
  httpheader_t ret = try_alloc_space_for (httpheader_t);
  raw_httpheader_t val = strchrnul (header, ':'),
                   crlf_pos = strchrnul (header, CRLF[0]);
  if (*crlf_pos == '\0')
    return result_with_error ("header is not CRLF-terminated");
  *crlf_pos = '\0';
  if (*val == '\0')
    return result_with_error ("header has no value separator");
  *val++ = '\0';
  ret->name = header;
  ret->value_as.raw = strip_whitespace (val);
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
    add_to_free_list (context, context->connection.accept);
    for (size_t i = 0; i < context->connection.accept->nr_items; i++)
      {
        add_to_free_list (context, context->connection.accept->items[i]);
        cb_debug ("accept item %zu: %s", i,
          context->connection.accept->items[i]->name);
      }
    break;
  }
case HTTPHEADER_KEEPALIVE:
  {
    httplist_t list = parse_http_list (header->value_as.raw, ',', '\0');
    add_to_free_list (context, list);
    size_t max, timeout;
    cb_debug ("parsing Keep-Alive header");
    for (size_t i = 0; i < list->nr_items; ++i)
      {
        struct http_list_item* item = list->items[i];
        add_to_free_list (context, item);
        if (item->value == NULL)
          return result_with_error ("keep-alive header has invalid format");
        if (!strcasecmp (item->name, "timeout"))
          {
            if (!parse_numeric (&context->connection.keep_alive.timeout,
                item->name))
              return result_with_error ("keep-alive timeout is non-numeric");
          }
        else if (!strcasecmp (item->name, "max"))
          if (!parse_numeric (&context->connection.keep_alive.max_reqs,
              item->name))
            return result_with_error ("keep-alive max is non-numeric");
      }
    break;
  }
case HTTPHEADER_ACCEPT_ENCODING:
  {
    cb_debug ("setting accept encoding to %s", header->value_as.raw); 
    httplist_t encodings = parse_http_list (header->value_as.raw, ',', ';');
    add_to_free_list (context, encodings);

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
}

struct __g_http_methods g_http_methods = {
  .parse_methodline = parse_methodline,
  .parse_headerline = parse_headerline,
  .create_context = create_context
};