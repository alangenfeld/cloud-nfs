
/*
 * Sun RPC is a product of Sun Microsystems, Inc. and is provided for
 * unrestricted use provided that this legend is included on all tape
 * media and as a part of the software program in whole or part.  Users
 * may copy or modify Sun RPC without charge, but are not authorized
 * to license or distribute it to anyone else except as part of a product or
 * program developed by the user.
 * 
 * SUN RPC IS PROVIDED AS IS WITH NO WARRANTIES OF ANY KIND INCLUDING THE
 * WARRANTIES OF DESIGN, MERCHANTIBILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE, OR ARISING FROM A COURSE OF DEALING, USAGE OR TRADE PRACTICE.
 * 
 * Sun RPC is provided with no support and without any obligation on the
 * part of Sun Microsystems, Inc. to assist in its use, correction,
 * modification or enhancement.
 * 
 * SUN MICROSYSTEMS, INC. SHALL HAVE NO LIABILITY WITH RESPECT TO THE
 * INFRINGEMENT OF COPYRIGHTS, TRADE SECRETS OR ANY PATENTS BY SUN RPC
 * OR ANY PART THEREOF.
 * 
 * In no event will Sun Microsystems, Inc. be liable for any lost revenue
 * or profits or other special, indirect and consequential damages, even if
 * Sun has been advised of the possibility of such damages.
 * 
 * Sun Microsystems, Inc.
 * 2550 Garcia Avenue
 * Mountain View, California  94043
 */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef _SOLARIS
#include "solaris_port.h"
#endif

#include <sys/cdefs.h>

/*
 * svc_vc.c, Server side for Connection Oriented based RPC. 
 *
 * Actually implements two flavors of transporter -
 * a tcp rendezvouser (a listner and connection establisher)
 * and a record/tcp stream.
 */
#include <pthread.h>
/* #include <reentrant.h> */

#include <sys/socket.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/poll.h>
#include <sys/un.h>
#include <sys/time.h>
#include <sys/uio.h>
#include <netinet/in.h>
#include <netinet/tcp.h>

#include <assert.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <rpc/rpc.h>
#include <Rpc_com_tirpc.h>
#include "stuff_alloc.h"
#include "RW_Lock.h"
#include <pthread.h>

int getpeereid(int s, uid_t * euid, gid_t * egid);

pthread_mutex_t mutex_cond_xprt[FD_SETSIZE];
pthread_cond_t condvar_xprt[FD_SETSIZE];
int etat_xprt[FD_SETSIZE];

extern SVCXPRT **Xports;
extern rw_lock_t Svc_fd_lock;
extern fd_set Svc_fdset;

extern void Xprt_register(SVCXPRT *);
extern void Xprt_unregister(SVCXPRT *);
extern void *rpc_tcp_socket_manager_thread(void *Arg);

static SVCXPRT *Makefd_xprt(int, u_int, u_int);
static bool_t Rendezvous_request(SVCXPRT *, struct rpc_msg *);
static enum xprt_stat Rendezvous_stat(SVCXPRT *);
static void Svc_vc_destroy(SVCXPRT *);
static void __Svc_vc_dodestroy(SVCXPRT *);
static int Read_vc(void *, void *, int);
static int Write_vc(void *, void *, int);
static enum xprt_stat Svc_vc_stat(SVCXPRT *);
static bool_t Svc_vc_recv(SVCXPRT *, struct rpc_msg *);
static bool_t Svc_vc_getargs(SVCXPRT *, xdrproc_t, void *);
static bool_t Svc_vc_freeargs(SVCXPRT *, xdrproc_t, void *);
static bool_t Svc_vc_reply(SVCXPRT *, struct rpc_msg *);
static void Svc_vc_rendezvous_ops(SVCXPRT *);
static void Svc_vc_ops(SVCXPRT *);
static bool_t Svc_vc_control(SVCXPRT * xprt, const u_int rq, void *in);
static bool_t Svc_vc_rendezvous_control(SVCXPRT * xprt, const u_int rq, void *in);

struct cf_rendezvous
{                               /* kept in xprt->xp_p1 for rendezvouser */
  u_int sendsize;
  u_int recvsize;
  int maxrec;
};

struct cf_conn
{                               /* kept in xprt->xp_p1 for actual connection */
  enum xprt_stat strm_stat;
  u_int32_t x_id;
  XDR xdrs;
  char verf_body[MAX_AUTH_BYTES];
  u_int sendsize;
  u_int recvsize;
  int maxrec;
  bool_t nonblock;
  struct timeval last_recv_time;
};

static void map_ipv4_to_ipv6(sin, sin6)
struct sockaddr_in *sin;
struct sockaddr_in6 *sin6;
{
  sin6->sin6_family = AF_INET6;
  sin6->sin6_port = sin->sin_port;
  sin6->sin6_addr.s6_addr32[0] = 0;
  sin6->sin6_addr.s6_addr32[1] = 0;
  sin6->sin6_addr.s6_addr32[2] = htonl(0xffff);
  sin6->sin6_addr.s6_addr32[3] = *(uint32_t *) & sin->sin_addr;
}

/*
 * Usage:
 *	xprt = svc_vc_create(sock, send_buf_size, recv_buf_size);
 *
 * Creates, registers, and returns a (rpc) tcp based transporter.
 * Once *xprt is initialized, it is registered as a transporter
 * see (svc.h, xprt_register).  This routine returns
 * a NULL if a problem occurred.
 *
 * The filedescriptor passed in is expected to refer to a bound, but
 * not yet connected socket.
 *
 * Since streams do buffered io similar to stdio, the caller can specify
 * how big the send and receive buffers are via the second and third parms;
 * 0 => use the system default.
 */
SVCXPRT *Svc_vc_create(fd, sendsize, recvsize)
int fd;
u_int sendsize;
u_int recvsize;
{
  SVCXPRT *xprt;
  struct cf_rendezvous *r = NULL;
  struct __rpc_sockinfo si;
  struct sockaddr_storage sslocal;
  socklen_t slen;

  r = (struct cf_rendezvous *)Mem_Alloc(sizeof(*r));
  if(r == NULL)
    {
      warnx("Svc_vc_create: out of memory");
      goto cleanup_svc_vc_create;
    }
  if(!__rpc_fd2sockinfo(fd, &si))
    return NULL;
  r->sendsize = __rpc_get_t_size(si.si_af, si.si_proto, (int)sendsize);
  r->recvsize = __rpc_get_t_size(si.si_af, si.si_proto, (int)recvsize);
  r->maxrec = __svc_maxrec;
  xprt = (SVCXPRT *) Mem_Alloc(sizeof(SVCXPRT));
  if(xprt == NULL)
    {
      warnx("Svc_vc_create: out of memory");
      goto cleanup_svc_vc_create;
    }
  xprt->xp_tp = NULL;
  xprt->xp_p1 = r;
  xprt->xp_p2 = NULL;
  xprt->xp_p3 = NULL;
  xprt->xp_verf = _null_auth;
  Svc_vc_rendezvous_ops(xprt);
  xprt->xp_port = (u_short) - 1;        /* It is the rendezvouser */
  xprt->xp_fd = fd;

  slen = sizeof(struct sockaddr_storage);
  listen(fd, SOMAXCONN);
  if(getsockname(fd, (struct sockaddr *)(void *)&sslocal, &slen) < 0)
    {
      warnx("Svc_vc_create: could not retrieve local addr");
      goto cleanup_svc_vc_create;
    }

  xprt->xp_ltaddr.maxlen = xprt->xp_ltaddr.len = sizeof(sslocal);
  xprt->xp_ltaddr.buf = Mem_Alloc((size_t) sizeof(sslocal));
  if(xprt->xp_ltaddr.buf == NULL)
    {
      warnx("Svc_vc_create: no mem for local addr");
      goto cleanup_svc_vc_create;
    }
  memcpy(xprt->xp_ltaddr.buf, &sslocal, (size_t) sizeof(sslocal));
  xprt->xp_rtaddr.maxlen = sizeof(struct sockaddr_storage);

  Xprt_register(xprt);

  return (xprt);
 cleanup_svc_vc_create:
  if(r != NULL)
    Mem_Free(r);
  return (NULL);
}

/*
 * Like svtcp_create(), except the routine takes any *open* UNIX file
 * descriptor as its first input.
 */
SVCXPRT *Svc_fd_create(fd, sendsize, recvsize)
int fd;
u_int sendsize;
u_int recvsize;
{
  struct sockaddr_storage ss;
  struct sockaddr_in6 sin6;
  socklen_t slen;
  SVCXPRT *ret;

  assert(fd != -1);

  ret = Makefd_xprt(fd, sendsize, recvsize);
  if(ret == NULL)
    return NULL;

  slen = sizeof(struct sockaddr_storage);
  if(getsockname(fd, (struct sockaddr *)(void *)&ss, &slen) < 0)
    {
      warnx("Svc_fd_create: could not retrieve local addr");
      goto freedata;
    }
  ret->xp_ltaddr.maxlen = ret->xp_ltaddr.len = sizeof(ss);
  ret->xp_ltaddr.buf = Mem_Alloc((size_t) sizeof(ss));
  if(ret->xp_ltaddr.buf == NULL)
    {
      warnx("Svc_fd_create: no mem for local addr");
      goto freedata;
    }
  memcpy(ret->xp_ltaddr.buf, &ss, (size_t) sizeof(ss));
  slen = sizeof(struct sockaddr_storage);
  if(getpeername(fd, (struct sockaddr *)(void *)&ss, &slen) < 0)
    {
      warnx("Svc_fd_create: could not retrieve remote addr");
      goto freedata;
    }
  if(ss.ss_family == AF_INET)
    {
      map_ipv4_to_ipv6((struct sockaddr_in *)&ss, &sin6);
    }
  else
    {
      memcpy(&sin6, &ss, sizeof(ss));
    }
  ret->xp_rtaddr.maxlen = ret->xp_rtaddr.len = sizeof(ss);
  ret->xp_rtaddr.buf = Mem_Alloc((size_t) sizeof(ss));
  if(ret->xp_rtaddr.buf == NULL)
    {
      warnx("Svc_fd_create: no mem for local addr");
      goto freedata;
    }
  if(ss.ss_family == AF_INET)
    memcpy(ret->xp_rtaddr.buf, &ss, (size_t) sizeof(ss));
  else
    memcpy(ret->xp_rtaddr.buf, &sin6, (size_t) sizeof(ss));
#ifdef PORTMAP
  if(sin6.sin6_family == AF_INET6 || sin6.sin6_family == AF_LOCAL)
    {
      memcpy(&ret->xp_raddr, ret->xp_rtaddr.buf, sizeof(struct sockaddr_in6));
      ret->xp_addrlen = sizeof(struct sockaddr_in6);
    }
#endif                          /* PORTMAP */

  return ret;

 freedata:
  if(ret->xp_ltaddr.buf != NULL)
    Mem_Free(ret->xp_ltaddr.buf);

  return NULL;
}

static SVCXPRT *Makefd_xprt(fd, sendsize, recvsize)
int fd;
u_int sendsize;
u_int recvsize;
{
  SVCXPRT *xprt;
  struct cf_conn *cd;
  const char *netid;
  struct __rpc_sockinfo si;

  assert(fd != -1);

  xprt = (SVCXPRT *) Mem_Alloc(sizeof(SVCXPRT));
  if(xprt == NULL)
    {
      warnx("svc_vc: Makefd_xprt: out of memory");
      goto done;
    }
  memset(xprt, 0, sizeof *xprt);
  cd = (struct cf_conn *)Mem_Alloc(sizeof(struct cf_conn));
  if(cd == NULL)
    {
      warnx("svc_tcp: Makefd_xprt: out of memory");
      Mem_Free(xprt);
      xprt = NULL;
      goto done;
    }
  cd->strm_stat = XPRT_IDLE;
  xdrrec_create(&(cd->xdrs), sendsize, recvsize, xprt, Read_vc, Write_vc);
  xprt->xp_p1 = cd;
  xprt->xp_verf.oa_base = cd->verf_body;
  Svc_vc_ops(xprt);             /* truely deals with calls */
  xprt->xp_port = 0;            /* this is a connection, not a rendezvouser */
  xprt->xp_fd = fd;
  if(__rpc_fd2sockinfo(fd, &si) && __rpc_sockinfo2netid(&si, &netid))
    xprt->xp_netid = strdup(netid);

  Xprt_register(xprt);
 done:
  return (xprt);
}

 /*ARGSUSED*/ static bool_t Rendezvous_request(xprt, msg)
SVCXPRT *xprt;
struct rpc_msg *msg;
{
  int sock, flags;
  struct cf_rendezvous *r;
  struct cf_conn *cd;
  struct sockaddr_storage addr;
  struct sockaddr_in6 sin6;
  socklen_t len;
  struct __rpc_sockinfo si;
  SVCXPRT *newxprt;
  fd_set cleanfds;

  pthread_attr_t attr_thr;
  pthread_t sockmgr_thrid;
  int rc = 0;

  assert(xprt != NULL);
  assert(msg != NULL);

  r = (struct cf_rendezvous *)xprt->xp_p1;
 again:
  len = sizeof(struct sockaddr_storage);
  if((sock = accept(xprt->xp_fd, (struct sockaddr *)(void *)&addr, &len)) < 0)
    {
      LogCrit(COMPONENT_DISPATCH, "Error in accept xp_fd=%u line=%u file=%s, errno=%u", xprt->xp_fd,
             __LINE__, __FILE__, errno);
      if(errno == EINTR)
        goto again;
      /*
       * Clean out the most idle file descriptor when we're
       * running out.
       */
      if(errno == EMFILE || errno == ENFILE)
        {
          cleanfds = Svc_fdset;
          __svc_clean_idle(&cleanfds, 0, FALSE);
          goto again;
        }
      return (FALSE);
    }
  /*
   * make a new transporter (re-uses xprt)
   */

  newxprt = Makefd_xprt(sock, r->sendsize, r->recvsize);
  if(addr.ss_family == AF_INET)
    {
      map_ipv4_to_ipv6((struct sockaddr_in *)&addr, &sin6);
    }
  else
    {
      memcpy(&sin6, &addr, len);
    }
  newxprt->xp_rtaddr.buf = Mem_Alloc(len);
  if(newxprt->xp_rtaddr.buf == NULL)
    return (FALSE);

  if(addr.ss_family == AF_INET)
    memcpy(newxprt->xp_rtaddr.buf, &addr, len);
  else
    memcpy(newxprt->xp_rtaddr.buf, &sin6, len);
  newxprt->xp_rtaddr.maxlen = newxprt->xp_rtaddr.len = len;
#ifdef PORTMAP
  if(sin6.sin6_family == AF_INET6 || sin6.sin6_family == AF_LOCAL)
    {
      memcpy(&newxprt->xp_raddr, newxprt->xp_rtaddr.buf, sizeof(struct sockaddr_in6));
      newxprt->xp_addrlen = sizeof(struct sockaddr_in6);
    }
#endif                          /* PORTMAP */
  if(__rpc_fd2sockinfo(sock, &si) && si.si_proto == IPPROTO_TCP)
    {
      len = 1;
      /* XXX fvdl - is this useful? */
      setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, &len, sizeof(len));
    }

  cd = (struct cf_conn *)newxprt->xp_p1;

  cd->recvsize = r->recvsize;
  cd->sendsize = r->sendsize;
  cd->maxrec = r->maxrec;

  if(cd->maxrec != 0)
    {
      flags = fcntl(sock, F_GETFL, 0);
      if(flags == -1)
        return (FALSE);
      /*if (fcntl(sock, F_SETFL, flags | O_NONBLOCK) == -1)
         return (FALSE); */
      if(cd->recvsize > cd->maxrec)
        cd->recvsize = cd->maxrec;
      cd->nonblock = TRUE;
      __xdrrec_setnonblock(&cd->xdrs, cd->maxrec);
    }
  else
    cd->nonblock = FALSE;
  gettimeofday(&cd->last_recv_time, NULL);

  /* Spawns a new thread to handle the connection */
  pthread_attr_init(&attr_thr);
  pthread_attr_setscope(&attr_thr, PTHREAD_SCOPE_SYSTEM);
  pthread_attr_setdetachstate(&attr_thr, PTHREAD_CREATE_DETACHED);      /* If not, the conn mgr will be "defunct" threads */

  FD_CLR(newxprt->xp_fd, &Svc_fdset);

  if(pthread_cond_init(&condvar_xprt[newxprt->xp_fd], NULL) != 0)
    return FALSE;

  /* Init the mutex */
  memset(&mutex_cond_xprt[newxprt->xp_fd], 0, sizeof(pthread_mutex_t));

  etat_xprt[newxprt->xp_fd] = 0;

  if((rc =
      pthread_create(&sockmgr_thrid, &attr_thr, rpc_tcp_socket_manager_thread,
                     (void *)(newxprt->xp_fd))) != 0)
    return FALSE;

  return (FALSE);               /* there is never an rpc msg to be processed */
}

 /*ARGSUSED*/ static enum xprt_stat Rendezvous_stat(xprt)
SVCXPRT *xprt;
{

  return (XPRT_IDLE);
}

static void Svc_vc_destroy(xprt)
SVCXPRT *xprt;
{
  assert(xprt != NULL);

  Xprt_unregister(xprt);
  __Svc_vc_dodestroy(xprt);
}

static void __Svc_vc_dodestroy(xprt)
SVCXPRT *xprt;
{
  struct cf_conn *cd;
  struct cf_rendezvous *r;

  cd = (struct cf_conn *)xprt->xp_p1;

  if(xprt->xp_fd != RPC_ANYFD)
    (void)close(xprt->xp_fd);
  if(xprt->xp_port != 0)
    {
      /* a rendezvouser socket */
      r = (struct cf_rendezvous *)xprt->xp_p1;
      Mem_Free(r);
      xprt->xp_port = 0;
    }
  else
    {
      /* an actual connection socket */
      XDR_DESTROY(&(cd->xdrs));
      Mem_Free(cd);
    }
  if(xprt->xp_rtaddr.buf)
    Mem_Free(xprt->xp_rtaddr.buf);
  if(xprt->xp_ltaddr.buf)
    Mem_Free(xprt->xp_ltaddr.buf);
  if(xprt->xp_tp)
    free(xprt->xp_tp);
  if(xprt->xp_netid)
    free(xprt->xp_netid);
  Mem_Free(xprt);
}

 /*ARGSUSED*/ static bool_t Svc_vc_control(xprt, rq, in)
SVCXPRT *xprt;
const u_int rq;
void *in;
{
  return (FALSE);
}

static bool_t Svc_vc_rendezvous_control(xprt, rq, in)
SVCXPRT *xprt;
const u_int rq;
void *in;
{
  struct cf_rendezvous *cfp;

  cfp = (struct cf_rendezvous *)xprt->xp_p1;
  if(cfp == NULL)
    return (FALSE);
  switch (rq)
    {
    case SVCGET_CONNMAXREC:
      *(int *)in = cfp->maxrec;
      break;
    case SVCSET_CONNMAXREC:
      cfp->maxrec = *(int *)in;
      break;
    default:
      return (FALSE);
    }
  return (TRUE);
}

/*
 * reads data from the tcp or uip connection.
 * any error is fatal and the connection is closed.
 * (And a read of zero bytes is a half closed stream => error.)
 * All read operations timeout after 35 seconds.  A timeout is
 * fatal for the connection.
 */
static int Read_vc(xprtp, buf, len)
void *xprtp;
void *buf;
int len;
{
  SVCXPRT *xprt;
  int sock;
  int milliseconds = 35 * 1000;
  struct pollfd pollfd;
  struct cf_conn *cfp;

  xprt = (SVCXPRT *) xprtp;
  assert(xprt != NULL);

  sock = xprt->xp_fd;

  cfp = (struct cf_conn *)xprt->xp_p1;

  if(cfp->nonblock)
    {
      len = read(sock, buf, (size_t) len);
      if(len < 0)
        {
          if(errno == EAGAIN)
            len = 0;
          else
            goto fatal_err;
        }
      if(len != 0)
        gettimeofday(&cfp->last_recv_time, NULL);
      return len;
    }

  do
    {
      pollfd.fd = sock;
      pollfd.events = POLLIN;
      pollfd.revents = 0;
      switch (poll(&pollfd, 1, milliseconds))
        {
        case -1:
          if(errno == EINTR)
            continue;
         /*FALLTHROUGH*/ case 0:
          goto fatal_err;

        default:
          break;
        }
    }
  while((pollfd.revents & POLLIN) == 0);

  if((len = read(sock, buf, (size_t) len)) > 0)
    {
      gettimeofday(&cfp->last_recv_time, NULL);
      return (len);
    }

 fatal_err:
  ((struct cf_conn *)(xprt->xp_p1))->strm_stat = XPRT_DIED;
  return (-1);
}

/*
 * writes data to the tcp connection.
 * Any error is fatal and the connection is closed.
 */
static int Write_vc(xprtp, buf, len)
void *xprtp;
void *buf;
int len;
{
  SVCXPRT *xprt;
  int i, cnt;
  struct cf_conn *cd;
  struct timeval tv0, tv1;

  xprt = (SVCXPRT *) xprtp;
  assert(xprt != NULL);

  cd = (struct cf_conn *)xprt->xp_p1;

  if(cd->nonblock)
    gettimeofday(&tv0, NULL);

  for(cnt = len; cnt > 0; cnt -= i, buf += i)
    {
      i = write(xprt->xp_fd, buf, (size_t) cnt);
      if(i < 0)
        {
          if(errno != EAGAIN || !cd->nonblock)
            {
              cd->strm_stat = XPRT_DIED;
              return (-1);
            }
          if(cd->nonblock && i != cnt)
            {
              /*
               * For non-blocking connections, do not
               * take more than 2 seconds writing the
               * data out.
               *
               * XXX 2 is an arbitrary amount.
               */
              gettimeofday(&tv1, NULL);
              if(tv1.tv_sec - tv0.tv_sec >= 2)
                {
                  cd->strm_stat = XPRT_DIED;
                  return (-1);
                }
            }
        }
    }

  return (len);
}

static enum xprt_stat Svc_vc_stat(xprt)
SVCXPRT *xprt;
{
  struct cf_conn *cd;

  assert(xprt != NULL);

  cd = (struct cf_conn *)(xprt->xp_p1);

  if(cd->strm_stat == XPRT_DIED)
    return (XPRT_DIED);
  if(!xdrrec_eof(&(cd->xdrs)))
    return (XPRT_MOREREQS);
  return (XPRT_IDLE);
}

static bool_t Svc_vc_recv(xprt, msg)
SVCXPRT *xprt;
struct rpc_msg *msg;
{
  struct cf_conn *cd;
  XDR *xdrs;

  assert(xprt != NULL);
  assert(msg != NULL);

  cd = (struct cf_conn *)(xprt->xp_p1);
  xdrs = &(cd->xdrs);

  if(cd->nonblock)
    {
      if(!__xdrrec_getrec(xdrs, &cd->strm_stat, TRUE))
        return FALSE;
    }

  xdrs->x_op = XDR_DECODE;
  (void)xdrrec_skiprecord(xdrs);
  if(xdr_callmsg(xdrs, msg))
    {
      cd->x_id = msg->rm_xid;
      return (TRUE);
    }
  cd->strm_stat = XPRT_DIED;
  return (FALSE);
}

static bool_t Svc_vc_getargs(xprt, xdr_args, args_ptr)
SVCXPRT *xprt;
xdrproc_t xdr_args;
void *args_ptr;
{

  assert(xprt != NULL);
  /* args_ptr may be NULL */
  return ((*xdr_args) (&(((struct cf_conn *)(xprt->xp_p1))->xdrs), args_ptr));
}

static bool_t Svc_vc_freeargs(xprt, xdr_args, args_ptr)
SVCXPRT *xprt;
xdrproc_t xdr_args;
void *args_ptr;
{
  XDR *xdrs;

  assert(xprt != NULL);
  /* args_ptr may be NULL */

  xdrs = &(((struct cf_conn *)(xprt->xp_p1))->xdrs);

  xdrs->x_op = XDR_FREE;
  return ((*xdr_args) (xdrs, args_ptr));
}

static bool_t Svc_vc_reply(xprt, msg)
SVCXPRT *xprt;
struct rpc_msg *msg;
{
  struct cf_conn *cd;
  XDR *xdrs;
  bool_t rstat;

  assert(xprt != NULL);
  assert(msg != NULL);

  cd = (struct cf_conn *)(xprt->xp_p1);
  xdrs = &(cd->xdrs);

  xdrs->x_op = XDR_ENCODE;
  msg->rm_xid = cd->x_id;
  rstat = xdr_replymsg(xdrs, msg);
  (void)xdrrec_endofrecord(xdrs, TRUE);
  return (rstat);
}

static void Svc_vc_ops(xprt)
SVCXPRT *xprt;
{
  static struct xp_ops ops;
  static struct xp_ops2 ops2;
  extern pthread_mutex_t ops_lock;

/* VARIABLES PROTECTED BY ops_lock: ops, ops2 */

  P(ops_lock);
  if(ops.xp_recv == NULL)
    {
      ops.xp_recv = Svc_vc_recv;
      ops.xp_stat = Svc_vc_stat;
      ops.xp_getargs = Svc_vc_getargs;
      ops.xp_reply = Svc_vc_reply;
      ops.xp_freeargs = Svc_vc_freeargs;
      ops.xp_destroy = Svc_vc_destroy;
      ops2.xp_control = Svc_vc_control;
    }
  xprt->xp_ops = &ops;
  xprt->xp_ops2 = &ops2;
  V(ops_lock);
}

static void Svc_vc_rendezvous_ops(xprt)
SVCXPRT *xprt;
{
  static struct xp_ops ops;
  static struct xp_ops2 ops2;
  extern pthread_mutex_t ops_lock;

  P(ops_lock);
  if(ops.xp_recv == NULL)
    {
      ops.xp_recv = Rendezvous_request;
      ops.xp_stat = Rendezvous_stat;
      ops.xp_getargs = (bool_t(*)(SVCXPRT *, xdrproc_t, void *))abort;
      ops.xp_reply = (bool_t(*)(SVCXPRT *, struct rpc_msg *))abort;
      ops.xp_freeargs =
          (bool_t(*)(SVCXPRT *, xdrproc_t, void *))abort, ops.xp_destroy = Svc_vc_destroy;
      ops2.xp_control = Svc_vc_rendezvous_control;
    }
  xprt->xp_ops = &ops;
  xprt->xp_ops2 = &ops2;
  V(ops_lock);
}

/*
 * Get the effective UID of the sending process. Used by rpcbind, keyserv
 * and rpc.yppasswdd on AF_LOCAL.
 */
int __rpc_get_local_uid(SVCXPRT * transp, uid_t * uid)
{
  int sock, ret;
  gid_t egid;
  uid_t euid;
  struct sockaddr *sa;

  sock = transp->xp_fd;
  sa = (struct sockaddr *)transp->xp_rtaddr.buf;
  if(sa->sa_family == AF_LOCAL)
    {
      ret = getpeereid(sock, &euid, &egid);
      if(ret == 0)
        *uid = euid;
      return (ret);
    }
  else
    return (-1);
}

/*
 * Destroy xprts that have not have had any activity in 'timeout' seconds.
 * If 'cleanblock' is true, blocking connections (the default) are also
 * cleaned. If timeout is 0, the least active connection is picked.
 */
bool_t __svc_clean_idle(fd_set * fds, int timeout, bool_t cleanblock)
{
  int i, ncleaned;
  SVCXPRT *xprt, *least_active;
  struct timeval tv, tdiff, tmax;
  struct cf_conn *cd;

  gettimeofday(&tv, NULL);
  tmax.tv_sec = tmax.tv_usec = 0;
  least_active = NULL;

  P_w(&Svc_fd_lock);
  for(i = ncleaned = 0; i <= svc_maxfd; i++)
    {
      if(FD_ISSET(i, fds))
        {
          xprt = Xports[i];
          if(xprt == NULL || xprt->xp_ops == NULL || xprt->xp_ops->xp_recv != Svc_vc_recv)
            continue;
          cd = (struct cf_conn *)xprt->xp_p1;
          if(!cleanblock && !cd->nonblock)
            continue;
          if(timeout == 0)
            {
              timersub(&tv, &cd->last_recv_time, &tdiff);
              if(timercmp(&tdiff, &tmax, >))
                {
                  tmax = tdiff;
                  least_active = xprt;
                }
              continue;
            }
          if(tv.tv_sec - cd->last_recv_time.tv_sec > timeout)
            {
              __Xprt_unregister_unlocked(xprt);
              __Svc_vc_dodestroy(xprt);
              ncleaned++;
            }
        }
    }
  if(timeout == 0 && least_active != NULL)
    {
      __Xprt_unregister_unlocked(least_active);
      __Svc_vc_dodestroy(least_active);
      ncleaned++;
    }
  V_w(&Svc_fd_lock);
  return ncleaned > 0 ? TRUE : FALSE;
}
