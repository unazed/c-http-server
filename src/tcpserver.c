#include "../include/tcpserver.h"
#include "../include/thunks.h"
#include <alloca.h>
#include <sys/epoll.h>
#include <fcntl.h>

inline static bool
__int_set_nonblocking (tcp_sockfd_t sockfd)
{
  int flags = fcntl (sockfd, F_GETFL, 0);
  debug ("trying to set socket (fd=%d) to non-blocking mode", sockfd);
  if (flags == -1)
    return false;
  flags |= O_NONBLOCK;
  return fcntl (sockfd, F_SETFL, flags) != -1;
}

inline static bool
__int_set_reuse_address (tcp_sockfd_t sockfd)
{
  debug ("trying to set SO_REUSE_ADDR on socket (fd=%d)", sockfd);
  int optval = 1;
  return setsockopt (
    sockfd, SOL_SOCKET, SO_REUSEADDR,
    &optval, sizeof (optval)
  ) != -1;
}

static struct __int_tcp_socket
__int_create_tcp_socket (void)
{
  tcp_sockfd_t sockfd = socket (AF_INET, SOCK_STREAM, 0);
  if (sockfd == -1)
    panic (
      "failed to create TCP socket"
    );
  debug ("created TCP socket (fd=%d)", sockfd);
  if (!__int_set_reuse_address (sockfd))
    panic (
      "failed to set SO_REUSEADDR on TCP socket (fd=%d)", sockfd
    );
  return (struct __int_tcp_socket) {
    .sockfd = sockfd,
    .is_blocking = !__int_set_nonblocking (sockfd)
  };
}

static tcpserver_t
__int_ts_bind_server (tcpserver_t server)
{
  struct sockaddr_in addr = {
    .sin_family = AF_INET,
    .sin_port = htons (server->__int_bind_info.port),
  };
  if (!inet_aton (server->__int_bind_info.address, &addr.sin_addr))
    panic (
      "failed to convert address '%s' to in_addr_t",
      server->__int_bind_info.address
    );
  if (bind (
        server->__int_stream.self.sockfd,
        (struct sockaddr*)&addr,
        sizeof (addr)
      ) == -1)
    panic (
      "%s() failed to bind TCP socket to address %s:%hu",
      __func__, server->__int_bind_info.address, server->__int_bind_info.port
    );
  debug (
    "bound TCP socket (fd=%d) to address %s:%hu",
    server->__int_stream.self.sockfd,
    server->__int_bind_info.address, server->__int_bind_info.port
  );
  return server;
}

__THUNK_DECL void
__int_set_recv_low_watermark (tcp_client_t self, size_t watermark)
{
  if (setsockopt (
      self->connection.sockfd, SOL_SOCKET, SO_RCVLOWAT,
      &watermark, sizeof (watermark)
      ))
    panic ("failed to set SO_RCVLOWAT to %zu", watermark);
  debug ("set SO_RCVLOWAT to %zu", watermark);
}

static inline recv_ret_t
__int_ts_generic_recv (tcp_client_t self, void* buf, size_t len, int flags)
{
  recv_ret_t ret;
  bool allocated_tmp_buf = false;

  debug ("trying to receive %zu bytes from socket (fd=%d, flags=%d)",
         len, self->connection.sockfd, flags);
  if (buf == NULL)
    {
      if (len > NULL_RECV_BUFFER_THRESHOLD)
        {
          warn ("allocating temporary heap (size=%zu) buffer",
                len);
          buf = calloc (sizeof (*buf), len);
          allocated_tmp_buf = true;
          if (buf == NULL)
            panic ("failed to allocate temporary buffer");
        }   
      else
        {
          warn ("allocating temporary stack buffer (size=%zu)", len);
          buf = alloca (len);
        }
    }
  if ((ret = recv (
      self->connection.sockfd, buf, len, flags
      )) == -1)
    panic ("failed to recv() from TCP socket");
  if (allocated_tmp_buf)
    {
      warn ("temporary heap buffer deallocated, read %d bytes", ret);
      free (buf);
    }
  return ret;
}

__THUNK_DECL recv_ret_t
__int_ts_recv (tcp_client_t self, void* buf, size_t len)
{
  return __int_ts_generic_recv (self, buf, len, 0);
}

__THUNK_DECL recv_ret_t
__int_ts_peek (tcp_client_t self, void* buf, size_t len)
{
  return __int_ts_generic_recv (self, buf, len, MSG_PEEK);
}

__THUNK_DECL send_ret_t
__int_ts_send (tcp_client_t self, void* buf, size_t len)
{
  send_ret_t ret;
  if ((ret = send (
      self->connection.sockfd, buf, len, 0
      )) == -1)
    panic ("failed to send() to TCP socket");
  return ret;
}

inline static void
__int_start_listening (tcpserver_t server)
{
  if (listen (
      server->__int_stream.self.sockfd,
      server->__int_bind_info.backlog
     ) == -1)
    panic (
      "failed to listen on TCP socket (fd=%d)",
      server->__int_stream.self.sockfd
    );
  debug (
    "started listening on TCP socket (fd=%d)",
    server->__int_stream.self.sockfd
  );
}

__THUNK_DECL
struct __int_tcp_conninfo
__int_ts_getaddr (tcp_client_t self)
{
  struct sockaddr_in addr;
  socklen_t addrlen = sizeof (addr);
  if (getpeername (
      self->connection.sockfd, (struct sockaddr*)&addr, &addrlen
    ) == -1)
    panic (
      "failed to get remote address of TCP socket (fd=%d)",
      self->connection.sockfd
    );
  debug (
    "got remote address of TCP socket (fd=%d) to %s:%hu",
    self->connection.sockfd, inet_ntoa (addr.sin_addr), ntohs (addr.sin_port)
  );
  return (struct __int_tcp_conninfo) {
    .address = inet_ntoa (addr.sin_addr),
    .port = ntohs (addr.sin_port)
  };
}

__THUNK_DECL void
__int_tcp_socket_free (tcp_client_t self)
{
  debug ("free()ing TCP socket thunk control structures (fd=%d)",
         self->connection.sockfd);

  /* make sure all thunks allocated in __int_ts_accept() are freed */
  g_thunks.deallocate_thunk (self->connection.cfg.set_recv_low_watermark);
  g_thunks.deallocate_thunk (self->connection.op.get_address);
  g_thunks.deallocate_thunk (self->connection.op.recv);
  g_thunks.deallocate_thunk (self->connection.op.peek);
  g_thunks.deallocate_thunk (self->connection.op.send);
  g_thunks.deallocate_thunk (self->connection.op.close);
  g_thunks.deallocate_thunk (self->connection.__int.free);
}

__THUNK_DECL void
__int_ts_socket_close (tcp_client_t self)
{
  debug ("closing TCP socket (fd=%d)", self->connection.sockfd);
  if (close (self->connection.sockfd) == -1)
    panic ("failed to close TCP socket (fd=%d)", self->connection.sockfd);
  self->connection.closed = true;
  self->connection.__int.free ();
}

tcp_client_t
__int_ts_accept (tcpserver_t server)
{
  tcp_sockfd_t sockfd = accept (
    server->__int_stream.self.sockfd,
    NULL, NULL
  );
  if (sockfd == -1)
    panic (
      "failed to accept TCP socket (fd=%d)",
      server->__int_stream.self.sockfd
    );
  tcp_client_t client = calloc (1, sizeof (struct __int_tcp_client));
  if (client == NULL)
    panic ("failed to allocate memory for TCP client");
  client->connection.sockfd = sockfd;
  client->connection.__int.free = g_thunks.allocate_thunk (
    "tcp_socket_free",
    __int_tcp_socket_free, client
  );
  client->connection.op.get_address = g_thunks.allocate_thunk (
    "socket_getaddr",
    __int_ts_getaddr, client
  );
  client->connection.op.recv = g_thunks.allocate_thunk (
    "socket_recv",
    __int_ts_recv, client
  );
  client->connection.op.send = g_thunks.allocate_thunk (
    "socket_send",
    __int_ts_send, client
  );
  client->connection.op.peek = g_thunks.allocate_thunk (
    "socket_peek",
    __int_ts_peek, client
  );
  client->connection.cfg.set_recv_low_watermark = g_thunks.allocate_thunk (
    "socket_set_recv_low_watermark",
    __int_set_recv_low_watermark, server
  );
  client->connection.op.close = g_thunks.allocate_thunk (
    "socket_close",
    __int_ts_socket_close, client
  );
  client->connection.closed = false;

  return client;
}

inline static tcp_client_t
__int_configure_client (tcp_client_t self)
{
  if (!__int_set_nonblocking (self->connection.sockfd))
    panic (
      "failed to set client TCP socket to non-blocking (fd=%d)",
      self->connection.sockfd
    );
  __auto_type conninfo = self->connection.op.get_address ();
  debug (
    "configured client TCP socket (fd=%d) at %s:%hu",
    self->connection.sockfd, conninfo.address, conninfo.port
  );
  self->info.address = conninfo.address;
  self->info.port = conninfo.port;
  return self;
}

__THUNK_DECL void
__int_ts_start_event_loop (tcpserver_t server)
{
  tcp_sockfd_t self_sockfd = server->__int_stream.self.sockfd;
  conn_backlog_t self_backlog = server->__int_bind_info.backlog;

  __int_start_listening (server);

  struct epoll_event event = {
    .data = {.fd = self_sockfd},
    .events = EPOLLIN
  }, events[self_backlog];
  poller_t poller = epoll_create1 (0);
  
  if (poller == -1)
    panic ("failed to create epoll instance");
  debug ("created epoll instance (fd=%d)", poller);
  
  if (epoll_ctl (
      poller, EPOLL_CTL_ADD, self_sockfd, &event
     ) == -1)
    panic (
      "failed to add TCP socket (fd=%d) to epoll instance (fd=%d)",
      self_sockfd, poller
    );

  for (;;)
    {
      int nr_fds = epoll_wait (poller, events, self_backlog, 0);
      if (__builtin_expect (nr_fds == -1, 0))
        panic ("failed to epoll_wait() on epoll instance (fd=%d)",
               poller);
      for (size_t i = 0; i < nr_fds; ++i)
        if (events[i].data.fd == self_sockfd)
          {
            tcp_client_t client = __int_configure_client(
              __int_ts_accept (server)
            );
            event.data.fd = client->connection.sockfd;
            event.data.ptr = client;
            event.events = EPOLLIN | EPOLLOUT | EPOLLET;

            if (epoll_ctl (
                poller, EPOLL_CTL_ADD, client->connection.sockfd,
                &event
                ) == -1)
              panic (
                "failed to add TCP socket (fd=%d) to epoll instance (fd=%d)",
                client->connection.sockfd, poller
              );

            debug ("added TCP socket (fd=%d) to epoll instance (fd=%d)",
                   client->connection.sockfd, poller);
            if (server->callbacks.client_connected != NULL)
              server->callbacks.client_connected (client);
            else
              warn ("no client connection callback registered");
          }
        else  /* if not server socket */
          {
            tcp_client_t client = events[i].data.ptr;
            debug ("got TCP socket (fd=%d) event on epoll instance (fd=%d)",
                   client->connection.sockfd, poller);
            /* this is preemptive; works if there is no user data in the
             * buffer, otherwise closing is handled at application level
             */
            ssize_t nr_read = recv (
              client->connection.sockfd, NULL, 1, MSG_PEEK
            );
            if (__builtin_expect (!nr_read, 0))
              {
                debug ("TCP socket (fd=%d) closed by peer, reason: %s",
                       client->connection.sockfd, strerror (errno));
                if (server->callbacks.client_disconnected != NULL)
                  server->callbacks.client_disconnected (client);
                else
                  warn ("no client disconnection callback registered");
                if (epoll_ctl (
                    poller, EPOLL_CTL_DEL, client->connection.sockfd,
                    NULL
                   ) == -1)
                  panic ("failed to remove TCP socket (fd=%d) from epoll "
                         "instance (fd=%d)", client->connection.sockfd, poller);
                client->connection.op.close ();
                continue;
              }
            
            if (events[i].events | EPOLLIN)
              {
                if (server->callbacks.client_readable != NULL)
                  server->callbacks.client_readable (client);
                else
                  warn ("no client readable callback registered");
              }
            
            if (events[i].events | EPOLLOUT)
              {
                if (server->callbacks.client_writable != NULL)
                  server->callbacks.client_writable (client);
                else
                  warn ("no client writable callback registered");
              }
          }
    }
}

static void
__int_allocate_thunks (tcpserver_t server)
{
  server->start_event_loop = __int_allocate_thunk (
    "tcp_start_event_loop",
    __int_ts_start_event_loop, server
  );
}

static tcpserver_t
__int_ts_create_tcpserver (tcp_address_t address, tcp_port_t port)
{
  tcpserver_t server = malloc (sizeof (*server));
  if (server == NULL)
    panic ("failed to allocate memory for server");
  debug ("allocated memory for TCP server");

  server->__int_bind_info.address = address;
  server->__int_bind_info.port = port;
  server->__int_bind_info.backlog = DEFAULT_TCP_BACKLOG;

  server->__int_stream.clients = NULL;
  server->__int_stream.nr_clients = 0;
  server->__int_stream.self = __int_create_tcp_socket ();

  debug ("allocating thunks for TCP server");
  __int_allocate_thunks (server);

  return server;
}

tcpserver_t
__int_ts_create_with_bind (tcp_address_t address, tcp_port_t port)
{
  return __int_ts_bind_server (
    __int_ts_create_tcpserver (address, port)
  ); /* +1 for chaining calls :) */
}

void
__int_ts_free (tcpserver_t server)
{
  debug ("free()ing TCP server");
  if (server->__int_stream.self.sockfd != -1)
    close (server->__int_stream.self.sockfd);
  free (server);
}

struct __g_tcpserver g_tcpserver = {
  .create_and_bind_to = __int_ts_create_with_bind,
  .free = __int_ts_free
};