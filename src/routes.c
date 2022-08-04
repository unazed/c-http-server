#include "../include/routes.h"
#include <string.h>

static inline
bool
__int_readto (FILE * f_route, char ** const into, unsigned char to)
{
  int chr;
  ssize_t length = 0;
  while ( !feof (f_route) && (chr = fgetc (f_route)))
  {
    ++length;
    if (chr == to)
      break;
    else if (chr == ' ' || chr == '\t')
      continue;
  }
  if (chr != to)
    return false;
  if (into == NULL)
    return true;
  *into = calloc (length, sizeof (**into));
  if (*into == NULL)
    panic ("failed token allocation in __int_readto()");
  /* flimsy parsing algorithm, works specifically for the format
   * defined in ./routes
   */
  fseek (f_route, ftell (f_route) - length, SEEK_SET);
  fread (*into, length - 1, sizeof (**into), f_route);
  fgetc (f_route);
  return true;
}

static enum __int_bf_parse_token
__int_expect (FILE * f_route, char ** const into,
              enum __int_bf_parse_token expecting)
{
  int chr;
  while ((chr = fgetc (f_route)) != EOF)
  {
    if ((chr == ' ' || chr == '\t')
        && (   (expecting & SEPARATOR)
            || (expecting & EXPRESSION)
            || (expecting & COMMENT)
            ))
      continue;
    if (chr == TOK_COMMENT)
      {
        if (!(expecting & COMMENT))
          panic ("parse error: got unexpected comment");
        __int_readto (f_route, NULL, '\n');
        return COMMENT;
      }
    if (chr == TOK_EXPRESSION)
      {
        if (!(expecting & EXPRESSION))
          panic ("parse error: got unexpected expression");
        if (!__int_readto (f_route, into, TOK_EXPRESSION))
          panic ("parse error: unterminated expression");
        return EXPRESSION;
      }
    if (chr == TOK_SEPARATOR)
      {
        if (!(expecting & SEPARATOR))
          panic ("parse error: got unexpected separator");
        return SEPARATOR;
      }
    if (expecting & IDENTIFIER)
      {
        __int_readto (f_route, into, '\n');
        return IDENTIFIER;
      }
  }
  return FEOF;
}

__THUNK_DECL bool
__int_match_thunk (const char *const expr, const char *const value)
{
  debug ("in __int_match_thunk(expr=%s, value=%s)", expr, value);
  /* naive implementation for the moment. in short future i intend to
   * support a simplistic regex capture group syntax, but for now,
   * this is sufficient.
   */
  return !strcmp (expr, value);
}

inline static route_match_fn
__int_create_match_thunk (const char *const expr)
{
  return g_thunks.allocate_thunk (
    "create_route_match",
    __int_match_thunk, (void *)expr
  );
}

static void
__int_parse_route_table_entries (route_table_t route_table, FILE * f_route)
{
  char *expression, *identifier;
  route_table->routes = malloc (sizeof (*route_table->routes));
  if (route_table->routes == NULL)
    panic ("failed initial route allocation in "
           "__int_parse_route_table_entries()");
  while (!feof (f_route))
  {
    if (__int_expect (f_route, &expression, COMMENT | EXPRESSION) == COMMENT)
      continue;
    __int_expect (f_route, NULL, SEPARATOR);
    if (__int_expect (f_route, &identifier, IDENTIFIER) == FEOF)
      break;
    route_table->routes[route_table->nr_routes++] = (struct __int_route){
      .handler = NULL,
      .match = __int_create_match_thunk (expression),
      .__int_ident = {
        .identifier = identifier,
        .expression = expression
      }
    };
    route_table->routes = realloc (
      route_table->routes,
      (1 + route_table->nr_routes) * sizeof (*route_table->routes)
    );
    if (route_table->routes == NULL)
      panic ("__int_parse_route_table_entries() failed to reallocate space "
             "for routes");
  }
  rewind (f_route);
}

inline static register_routes_fn
__int_create_register_routes_thunk (route_table_t route_table)
{
  register_routes_fn thunk = g_thunks.allocate_thunk (
    "register_routes",
    __int_register_routes_thunk,
    route_table
  );
  debug ("register_routes() is now @ %p", thunk);
  return thunk; 
}

static route_table_t
__int_fromfile (FILE * f_route)
{
  route_table_t route_table;
  if ((route_table = malloc (sizeof (struct __int_route_table))) == NULL)
    panic ("malloc() failed allocating route table");
  __int_parse_route_table_entries (route_table, f_route);
  route_table->register_routes = __int_create_register_routes_thunk (
    route_table
  );
  return route_table;
}

void
__int_rt_free (route_table_t route_table)
{
  debug ("free()ing route table");
  for (size_t i = 0; i < route_table->nr_routes; ++i)
    {
      struct __int_route route = route_table->routes[i];
      debug ("free()ing route (%s, %s)", route.__int_ident.expression,
        route.__int_ident.identifier);
      free (route.__int_ident.expression);
      free (route.__int_ident.identifier);
    }
  free (route_table->routes);
  free (route_table);
}

__attribute__((noinline))
static struct __int_route*
__int_find_route (route_table_t route_table, const char *const name)
{
  for (size_t i = 0; i < route_table->nr_routes; ++i)
    if (!strcmp (route_table->routes[i].__int_ident.identifier, name))
      return &route_table->routes[i];
  return NULL;
}

__THUNK_DECL void
__int_register_routes_thunk (
  route_table_t route_table,
  const struct route_table_entry* const route_table_map
  )
{
  for (size_t i = 0; route_table_map[i].name != NULL; ++i)
    {
      struct route_table_entry entry = route_table_map[i];
      debug ("trying to find route entry: '%s'", entry.name);
      struct __int_route* route;
      if ((route = __int_find_route (route_table, entry.name)) == NULL)
        panic ("failed to find entry in route table for '%s'", entry.name);
      route->handler = entry.function;
    }
}

struct __g_route_parser g_route_parser = {
  .from_file = __int_fromfile,
  .free = __int_rt_free
};