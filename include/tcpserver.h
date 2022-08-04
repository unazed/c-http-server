#ifndef __TCP_SERVER_H
#define __TCP_SERVER_H

#include "common.h"
#include "thunks.h"
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdbool.h>
#include <errno.h>
#include <string.h>

#define DEFAULT_TCP_BACKLOG (8)
#if DEFAULT_TCP_BACKLOG <= 0
# pragma GCC error "DEFAULT_TCP_BACKLOG must be positive"
#endif
#define NULL_RECV_BUFFER_THRESHOLD (512)
#if NULL_RECV_BUFFER_THRESHOLD <= 0
# pragma GCC error "NULL_RECV_BUFFER_THRESHOLD must be positive"
#endif

typedef typeof (socket (SOCK_STREAM, AF_INET, 0)) tcp_sockfd_t;
typedef typeof (recv (0, NULL, 0, 0)) recv_ret_t;
typedef typeof (send (0, NULL, 0, 0)) send_ret_t;

/* thunk typedef stubs */
typedef void (*__int_set_recv_low_watermark_fn)(size_t watermark);
typedef recv_ret_t (*__int_recv_fn)(void* buf, size_t len);
typedef recv_ret_t (*__int_peek_fn)(void* buf, size_t len);
typedef send_ret_t (*__int_send_fn)(void* buf, size_t len);
typedef void (*__int_close_fn)(void);
typedef void (*__int_ts_start_event_loop_fn)(void);
typedef void (*__int_tcp_socket_free_fn)(void);

struct __int_tcp_conninfo
{
  const char* address;
  unsigned short port;
};

typedef struct __int_tcp_conninfo (*__int_getaddr_fn)(void);

/* we couple address/port types with the TCP client structure intentionally */
typedef typeof (((struct __int_tcp_conninfo*)NULL)->address) tcp_address_t;
typedef typeof (((struct __int_tcp_conninfo*)NULL)->port) tcp_port_t;

struct __int_tcp_socket
{
  struct
  {
    __int_tcp_socket_free_fn free;
  } __int;
  struct
  {
    __int_recv_fn recv;
    __int_send_fn send;
    __int_peek_fn peek;
    __int_close_fn close;
    __int_getaddr_fn get_address;
  } op;
  struct
  {
    __int_set_recv_low_watermark_fn set_recv_low_watermark;
  } cfg;
  tcp_sockfd_t sockfd;
  bool is_blocking;
  bool closed;
};

typedef struct __int_tcp_client
{
  struct
  {
    tcp_address_t address;
    tcp_port_t port;
  } info;
  struct __int_tcp_socket connection;
} *tcp_client_t;

typedef int conn_backlog_t;
typedef int poller_t;

typedef void (*__int_callback_t)(tcp_client_t who);

typedef struct
{
  struct
  {
    tcp_address_t address;
    tcp_port_t port;
    conn_backlog_t backlog;
  } __int_bind_info;
  struct
  {
    struct
    {
      struct __int_tcp_client* clients;
      size_t nr_clients;
    };
    struct __int_tcp_socket self;
  } __int_stream;
  struct 
  {
    __int_callback_t client_connected;
    __int_callback_t client_disconnected;
    __int_callback_t client_readable;
    __int_callback_t client_writable;
  } callbacks;
  __int_ts_start_event_loop_fn start_event_loop;
}* tcpserver_t;

/* thunks */
__THUNK_DECL void __int_set_recv_low_watermark (tcp_client_t self,
  size_t watermark);
__THUNK_DECL recv_ret_t __int_ts_recv (tcp_client_t self, void* buf,
  size_t len);
__THUNK_DECL recv_ret_t __int_ts_send (tcp_client_t self, void* buf,
  size_t len);
__THUNK_DECL void __int_ts_start_event_loop (tcpserver_t server);
__THUNK_DECL struct __int_tcp_conninfo __int_ts_getaddr (tcp_client_t self);
__THUNK_DECL void __int_tcp_socket_free (tcp_client_t self);
__THUNK_DECL void __int_ts_socket_close (tcp_client_t self);

static struct __int_tcp_socket __int_create_tcp_socket (void);

tcpserver_t __int_ts_create_with_bind (tcp_address_t address, tcp_port_t port);
void __int_ts_free (tcpserver_t server);

struct __g_tcpserver {
  typeof (__int_ts_create_with_bind)* create_and_bind_to;
  typeof (__int_ts_free)* free;
};

extern struct __g_tcpserver g_tcpserver;

#endif /* __TCP_SERVER_H */
