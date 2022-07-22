#ifndef __ROUTES_H
#define __ROUTES_H

#undef PANIC_FORMAT_PREFIX
#define PANIC_FORMAT_PREFIX ("(internal error) ")

#include "common.h"
#include "thunks.h"
#include <stdbool.h>
#include <stdio.h>
#include <stddef.h>
#include <ctype.h>
#include <stdint.h>

#define ROUTE_FUNCTION(name) void name(void)

typedef bool (*route_match_fn)(const char* path);
typedef void (*route_handler_fn)(void /* TODO */);

struct __int_route
{
  route_match_fn match;
  route_handler_fn handler;
  struct {
    char* identifier;
    char* expression;
  } __int_ident;
};

struct route_table_entry
{
  const char* name;
  route_handler_fn function;
};

typedef void (*register_routes_fn)(const struct route_table_entry* const);

typedef struct __int_route_table
{
  struct __int_route* routes;
  size_t nr_routes;
  register_routes_fn register_routes;
} *route_table_t;

enum __int_bf_parse_token
{
  COMMENT     = 1,
  EXPRESSION  = 2,
  IDENTIFIER  = 4,
  SEPARATOR   = 8,
  FEOF        = 16
};

#define TOK_COMMENT (';')
#define TOK_SEPARATOR (':')
#define TOK_EXPRESSION ('"')

__THUNK_DECL void
__int_register_routes_thunk (route_table_t route_table,
  const struct route_table_entry* const route_table_map);

__THUNK_DECL bool
__int_match_thunk (const char *const expr, const char *const value);

static inline
bool __int_readto (FILE * f_route, char ** const into, unsigned char to);

static enum __int_bf_parse_token __int_expect (FILE * f_route,
  char ** const into, enum __int_bf_parse_token expecting);

static void __int_parse_route_table_entries (route_table_t route_table,
  FILE * f_route);

inline static register_routes_fn __int_create_register_routes_thunk (
  route_table_t route_table);

static route_match_fn __int_create_match_thunk (const char * const expr);

static route_table_t __int_fromfile (FILE * f_route);

static void __int_rt_free (route_table_t route_table);

struct __g_route_parser {
  typeof (__int_fromfile)* from_file;
  typeof (__int_rt_free)* free;
};

extern struct __g_route_parser g_route_parser;

#endif /* __ROUTES_H */