#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include "../include/common.h"
#include "../include/routes.h"
#include "../include/httpserver.h"

ROUTE_FUNCTION(route_index)
{
}

ROUTE_FUNCTION(route_wildcard)
{
}

ROUTE_FUNCTION(route_test_wildcard)
{
}

static const struct route_table_entry route_table_map[] = {
  {.name = "route_index", .function = route_index},
  {.name = "route_wildcard", .function = route_wildcard},
  {.name = "route_test_wildcard", .function = route_test_wildcard},
  {NULL, NULL}
};

static inline bool
check_exists_route_file (const char* const path)
{
  return access (path, F_OK) != -1;
}

static route_table_t route_table;
static httpserver_t server;

void
kbint_handler (int signum)
{
  printf ("\b\b");  /* backspace to erase the '^C' */
  log ("keyboard interrupt caught, exiting...");
  g_httpserver.free (server);
  g_route_parser.free (route_table);
  exit (EXIT_SUCCESS);
}

int
main (int argc, const char ** argv)
{
  if (argc != 4)
    panic ("usage: %s [path-to-routes: str] [host: str] [port: u16]",
           argv[0]);
  
  const char *path_to_routes = argv[1],
             *host = argv[2];
  const int port = atoi (argv[3]);
  if (port <= 0 || port > 65535)
    panic ("port not in range 0..65536");

  if (!check_exists_route_file (path_to_routes))
    panic ("route file: '%s' couldn't be found", path_to_routes);

  FILE * f_routes = fopen (path_to_routes, "rb");
  route_table = g_route_parser.from_file (f_routes);
  fclose (f_routes);
  log ("successfully parsed %zu routes from '%s'",
       route_table->nr_routes, path_to_routes);
  route_table->register_routes (route_table_map);
  log ("registered all routes to their corresponding handlers");
  log ("attempting to create and bind HTTP server to '%s:%d'", host, port);

  server = g_httpserver.create_and_bind_to (host, port);
  server->set_route_table (route_table);

  if (signal (SIGINT, kbint_handler) == SIG_IGN)
    signal (SIGINT, SIG_IGN);

  server->start_event_loop ();

  log ("all done, deallocating resources & exiting...");
  g_httpserver.free (server);
  g_route_parser.free (route_table);
  log ("done, goodbye!");
  return EXIT_SUCCESS;
}
