#define _GNU_SOURCE
#include "../include/httpimpl.h"
#include "../include/restype.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

static result_type_of (httpmethodline_t) 
parse_methodline (raw_httpheader_t methodline)
{
  if (methodline == NULL)
    return result_with_error ("methodline is NULL");
  httpmethodline_t ret = calloc (1, sizeof (*(httpmethodline_t)(0)));
  if (ret == NULL)
    return result_with_error ("methodline malloc() failed");
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

static result_type_of (httpheader_t)
parse_headerline (raw_httpheader_t header)
{
}

struct __g_http_methods g_http_methods = {
  .parse_methodline = parse_methodline,
  .parse_headerline = parse_headerline
};