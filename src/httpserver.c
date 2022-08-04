#include "../include/httpserver.h"
#include "../include/thunks.h"

__THUNK_DECL void
__int_set_route_table_thunk (httpserver_t this, route_table_t route_table)
{
  debug ("set route table for HTTP server");
  this->__int.route_table = route_table;

}

__THUNK_DECL void
__int_start_event_loop_thunk (httpserver_t this)
{
  if (this->__int.route_table == NULL)
    panic ("route table was not set");
  debug ("starting event loop for HTTP server");
  this->__int.tcp_server->start_event_loop ();
}

static void
__int_allocate_thunks (httpserver_t server)
{
  server->set_route_table = g_thunks.allocate_thunk (
    "http_set_route_table",
    __int_set_route_table_thunk,
    server
  );
  server->start_event_loop = g_thunks.allocate_thunk (
    "http_start_event_loop",
    __int_start_event_loop_thunk,
    server    
  );
  debug ("registering TCP callback thunks");
  __int_cb_register_callbacks (server);
  debug ("all thunks allocated on HTTP server instance");
}

httpserver_t
__int_hs_create_with_bind (tcp_address_t host, tcp_port_t port)
{
  httpserver_t server = malloc (sizeof (struct __int_httpserver));
  if (server == NULL)
    panic ("malloc() failed to allocate HTTP server instance");
  server->__int.route_table = NULL;
  debug ("allocated HTTP server instance, creating TCP server");
  server->__int.tcp_server = g_tcpserver.create_and_bind_to (host, port);
  debug ("allocating HTTP method thunks");
  __int_allocate_thunks (server);
  return server;
}

void
__int_hs_free (httpserver_t server)
{
  debug ("free()ing HTTP server");
  __int_ts_free (server->__int.tcp_server);
  free (server);
}

struct __g_httpserver g_httpserver = {
  .create_and_bind_to = __int_hs_create_with_bind,
  .free = __int_hs_free
};