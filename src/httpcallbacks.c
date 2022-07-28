#include "../include/httpserver.h"
#include "../include/httpimpl.h"
#include "../include/thunks.h"
#include "../include/common.h"
#include "../include/restype.h"

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

static recv_ret_t sz_initial_recv = ALG_INCR_INITIAL_RECV;

static recv_ret_t
__int_sk_incremental_find (
  tcp_client_t from, char ** into, const char *const pattern)
{
  size_t sz_pattern = strlen (pattern);
  if (!sz_pattern)
    panic ("search pattern is empty");
  char* search_buffer = malloc (sz_initial_recv);
  recv_ret_t sz_recv = sz_initial_recv,
             offset = 0, pos = -1;
  if (search_buffer == NULL)
    panic ("failed to allocate search buffer (size = %d)", sz_initial_recv);
  while (true)
    {
      recv_ret_t nr_recv = from->connection.op.peek (search_buffer, sz_recv);
      if (!nr_recv)
        return -1;
      for (size_t i = offset; i < nr_recv; ++i)
        {
          if (strncmp (&search_buffer[i], pattern, sz_pattern))
            {
              if ((search_buffer[i] == pattern[0])
                  && (i >= nr_recv - sz_pattern - 1))
                {
                  offset = i - sz_pattern;
                  break;
                }
              else
                offset = i;
              continue;
            }
          else
            {
              search_buffer[i + sz_pattern] = '\0';
              *into = search_buffer;
              sz_initial_recv = (sz_initial_recv + nr_recv) / 2;
              cb_debug ("incrementing initial recv size from %d to %d",
                sz_initial_recv * 2 - nr_recv, sz_initial_recv);
              return i + sz_pattern;
            }
        }
      sz_recv = sz_recv / 2 + nr_recv;
      search_buffer = realloc (search_buffer, sz_recv);
    }
  if (pos == -1)
    free (search_buffer);
  return pos;
}

static raw_httpheader_t
__int_http_read_header_line (tcp_client_t from)
{
  raw_httpheader_t header;
  recv_ret_t sz_header = __int_sk_incremental_find (from, &header, "\r\n");
  if (sz_header == -1)
    return NULL;
  cb_debug ("size of header = %zu", sz_header);
  from->connection.op.recv (NULL, sz_header);
  return header;
}

__THUNK_DECL void
__int_cb_client_connected (httpserver_t this, tcp_client_t who)
{
  cb_debug ("client connected: %s:%d", who->info.address, who->info.port);
}

__THUNK_DECL void
__int_cb_client_disconnected (httpserver_t this, tcp_client_t who)
{
  cb_debug ("client disconnected: %p", who);
}



__THUNK_DECL void
__int_cb_client_readable (httpserver_t this, tcp_client_t who)
{
  void
  when_parser_fails (const char* msg, httpmethodline_t methodline)
  {
    cb_error ("HTTP request failed to parse: '%s'", msg);
    who->connection.op.close ();
    /* this may double-free when malloc() already failed in
     * `g_http_methods.parse_methodline`, but at that point
     * we're already in a bad state, and we can accept our fate
     */
    free (methodline);
  }

  httpmethodline_t method_line = try_unwrap (
    g_http_methods.parse_methodline (__int_http_read_header_line (who)),
    (result_action_t){ .otherwise = when_parser_fails,
                       .aux_data = method_line });
  if (method_line == NULL)
    return;

}

__THUNK_DECL void
__int_cb_client_writable (httpserver_t this, tcp_client_t who)
{
  cb_debug ("client writable: %p", who);
}