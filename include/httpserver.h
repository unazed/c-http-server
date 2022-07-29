#ifndef __HTTP_SERVER_H
#define __HTTP_SERVER_H

#include "common.h"
#include "routes.h"
#include "tcpserver.h"
#include "thunks.h"

#define ALG_INCR_INITIAL_RECV (4)
#if ALG_INCR_INITIAL_RECV <= 0
# pragma GCC error ALG_INCR_INITIAL_RECV must be positive
#endif

typedef void (*__int_set_route_table_fn)(route_table_t route_table);
typedef void (*__int_hs_start_event_loop_fn)(void);

typedef struct __int_httpserver {
  struct {
    route_table_t route_table;
    tcpserver_t tcp_server;
  } __int;
  __int_set_route_table_fn set_route_table;
  __int_hs_start_event_loop_fn start_event_loop;
} *httpserver_t;

__THUNK_DECL void __int_set_route_table_thunk (httpserver_t this,
  route_table_t route_table);
__THUNK_DECL void __int_start_event_loop_thunk (httpserver_t this);
/* callbacks, impl. in: src/httpcallbacks.c */
void __int_cb_register_callbacks (httpserver_t server);
__THUNK_DECL void __int_cb_client_connected (httpserver_t this,
  tcp_client_t who);
__THUNK_DECL void __int_cb_client_disconnected (httpserver_t this,
  tcp_client_t who);
__THUNK_DECL void __int_cb_client_readable (httpserver_t this,
  tcp_client_t who);
__THUNK_DECL void __int_cb_client_writable (httpserver_t this,
  tcp_client_t who);

httpserver_t __int_hs_create_with_bind (tcp_address_t host, tcp_port_t port);
void __int_hs_free (httpserver_t server);

struct __g_httpserver
{
  typeof (__int_hs_create_with_bind)* create_and_bind_to;
  typeof (__int_hs_free)* free;
};

extern struct __g_httpserver g_httpserver;

#endif /* __HTTP_SERVER_H */
