#include "../include/httpserver.h"
#include "../include/thunks.h"
#include "../include/common.h"

extern inline void
__int_cb_register_callbacks (httpserver_t server)
{
  server->__int.tcp_server->callbacks.client_connected =
    __int_allocate_thunk (__int_cb_client_connected, server);
  server->__int.tcp_server->callbacks.client_disconnected =
    __int_allocate_thunk (__int_cb_client_disconnected, server);
  server->__int.tcp_server->callbacks.client_readable =
    __int_allocate_thunk (__int_cb_client_readable, server);
  server->__int.tcp_server->callbacks.client_writable =
    __int_allocate_thunk (__int_cb_client_writable, server);
}

__THUNK_DECL void
__int_cb_client_connected (httpserver_t this, tcp_client_t who)
{
  cb_debug ("client connected: %p", who);
}

__THUNK_DECL void
__int_cb_client_disconnected (httpserver_t this, tcp_client_t who)
{
  cb_debug ("client disconnected: %p", who);
}

__THUNK_DECL void
__int_cb_client_readable (httpserver_t this, tcp_client_t who)
{
  who->connection.op.close ();
}

__THUNK_DECL void
__int_cb_client_writable (httpserver_t this, tcp_client_t who)
{
  cb_debug ("client writable: %p", who);
}