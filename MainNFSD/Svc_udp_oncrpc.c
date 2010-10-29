#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef _SOLARIS
#include "solaris_port.h"
#ifndef _USE_SNMP
typedef unsigned long u_long;
#endif
typedef unsigned int u_int32_t;
#define _RPC_SVC_MT_H
#endif

#include   <unistd.h>
#include   <stdio.h>
#include   <stdlib.h>
#include   <string.h>
#include   "stuff_alloc.h"

#ifdef _USE_GSSRPC
#include   <gssrpc/rpc.h>
#include   <gssrpc/auth.h>
#include   <gssrpc/svc.h>
#else
#include   <rpc/rpc.h>
#include   <rpc/auth.h>
#include   <rpc/svc.h>
#endif

#include   <sys/poll.h>
#include   <sys/socket.h>
#include   <errno.h>

#ifdef _APPLE
#define MAX(a, b)   ((a) > (b)? (a): (b))
#endif

#ifndef UDPMSGSIZE
#define UDPMSGSIZE 8800
#endif

#ifndef MAX
#define MAX(a, b)     ((a > b) ? a : b)
#endif

#define rpc_buffer(xprt) ((xprt)->xp_p1)

void Xprt_register(SVCXPRT * xprt);
void Xprt_unregister(SVCXPRT * xprt);
bool_t svcauth_wrap_dummy(XDR * xdrs, xdrproc_t xdr_func, caddr_t xdr_ptr);

#define SVCAUTH_WRAP(auth, xdrs, xfunc, xwhere) svcauth_wrap_dummy( xdrs, xfunc, xwhere)
#define SVCAUTH_UNWRAP(auth, xdrs, xfunc, xwhere) svcauth_wrap_dummy( xdrs, xfunc, xwhere)

static bool_t Svcudp_recv();
static bool_t Svcudp_reply();
static enum xprt_stat Svcudp_stat();
static bool_t Svcudp_getargs();
static bool_t Svcudp_freeargs();
static void Svcudp_destroy();

static struct xp_ops Svcudp_op = {
  Svcudp_recv,
  Svcudp_stat,
  Svcudp_getargs,
  Svcudp_reply,
  Svcudp_freeargs,
  Svcudp_destroy
};

/*
 * kept in xprt->xp_p2
 */
struct Svcudp_data
{
  u_int su_iosz;                /* byte size of send.recv buffer */
  u_long su_xid;                /* transaction id */
  XDR su_xdrs;                  /* XDR handle */
  char su_verfbody[MAX_AUTH_BYTES];     /* verifier body */
};
#define	Su_data(xprt)	((struct Svcudp_data *)(xprt->xp_p2))

/*
 * Usage:
 *	xprt = Svcudp_create(sock);
 *
 * If sock<0 then a socket is created, else sock is used.
 * If the socket, sock is not bound to a port then Svcudp_create
 * binds it to an arbitrary port.  In any (successful) case,
 * xprt->xp_sock is the registered socket number and xprt->xp_port is the
 * associated port number.
 * Once *xprt is initialized, it is registered as a transporter;
 * see (svc.h, xprt_register).
 * The routines returns NULL if a problem occurred.
 */

SVCXPRT *Svcudp_bufcreate(register int sock, u_int sendsz, u_int recvsz)
{
  bool_t madesock = FALSE;
  register SVCXPRT *xprt;
  register struct Svcudp_data *su;
  struct sockaddr_in addr;
  unsigned long len = sizeof(struct sockaddr_in);

  if(sock == RPC_ANYSOCK)
    {
      if((sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
        {
          perror("Svcudp_create: socket creation problem");
          return ((SVCXPRT *) NULL);
        }
      madesock = TRUE;
    }

  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  if(bindresvport(sock, &addr))
    {
      addr.sin_port = 0;
      (void)bind(sock, (struct sockaddr *)&addr, len);
    }

  if(getsockname(sock, (struct sockaddr *)&addr, (socklen_t *) & len) != 0)
    {
      perror("Svcudp_create - cannot getsockname");
      if(madesock)
        (void)close(sock);
      return ((SVCXPRT *) NULL);
    }

  xprt = (SVCXPRT *) Mem_Alloc(sizeof(SVCXPRT));
  if(xprt == NULL)
    {
      return (NULL);
    }

  su = (struct Svcudp_data *)Mem_Alloc(sizeof(*su));

  if(su == NULL)
    {
      return (NULL);
    }
  su->su_iosz = ((MAX(sendsz, recvsz) + 3) / 4) * 4;
  if((rpc_buffer(xprt) = Mem_Alloc(su->su_iosz)) == NULL)
    {
      return (NULL);
    }

  xdrmem_create(&(su->su_xdrs), rpc_buffer(xprt), su->su_iosz, XDR_DECODE);
  xprt->xp_p2 = (caddr_t) su;
  xprt->xp_verf.oa_base = su->su_verfbody;
  xprt->xp_ops = &Svcudp_op;
  xprt->xp_port = ntohs(addr.sin_port);
#ifdef _FREEBSD
  xprt->xp_fd = sock;
#else
  xprt->xp_sock = sock;
#endif

  Xprt_register(xprt);

  return (xprt);
}

/*  */
SVCXPRT *Svcudp_create(int sock)
{

  return (Svcudp_bufcreate(sock, UDPMSGSIZE, UDPMSGSIZE));
}

void Svcudp_soft_destroy(register SVCXPRT * xprt)
{
  register struct Svcudp_data *su = Su_data(xprt);

  //XDR_DESTROY(&(su->su_xdrs));
  Mem_Free(rpc_buffer(xprt));
  Mem_Free((caddr_t) su);
  Mem_Free((caddr_t) xprt);
}

static void Svcudp_destroy(register SVCXPRT * xprt)
{
  register struct Svcudp_data *su = Su_data(xprt);

  Xprt_unregister(xprt);
#ifdef _FREEBSD
  (void)close(xprt->xp_fd);
#else
  (void)close(xprt->xp_sock);
#endif
  XDR_DESTROY(&(su->su_xdrs));
  Mem_Free(rpc_buffer(xprt));
  Mem_Free((caddr_t) su);
  Mem_Free((caddr_t) xprt);

}

static enum xprt_stat Svcudp_stat(SVCXPRT * xprt)
{

  return (XPRT_IDLE);
}

static bool_t Svcudp_recv(register SVCXPRT * xprt, struct rpc_msg *msg)
{
  register struct Svcudp_data *su = Su_data(xprt);
  register XDR *xdrs = &(su->su_xdrs);
  register int rlen;

 again:
  xprt->xp_addrlen = sizeof(struct sockaddr_in);
#ifdef _FREEBSD
  rlen = recvfrom(xprt->xp_fd, rpc_buffer(xprt), (int)su->su_iosz,
                  0, (struct sockaddr *)&(xprt->xp_raddr), &(xprt->xp_addrlen));
#else
  rlen = recvfrom(xprt->xp_sock, rpc_buffer(xprt), (int)su->su_iosz,
                  0, (struct sockaddr *)&(xprt->xp_raddr), &(xprt->xp_addrlen));
#endif

  if(rlen == -1 && errno == EINTR)
    goto again;

  if(rlen == -1 || rlen < 4 * sizeof(u_int32_t))
    return (FALSE);

  xdrs->x_op = XDR_DECODE;

  XDR_SETPOS(xdrs, 0);

  if(!xdr_callmsg(xdrs, msg))
    return (FALSE);

  su->su_xid = msg->rm_xid;

  return (TRUE);
}

static bool_t Svcudp_reply(register SVCXPRT * xprt, struct rpc_msg *msg)
{
  register struct Svcudp_data *su = Su_data(xprt);
  register XDR *xdrs = &(su->su_xdrs);
  register int slen;
  xdrproc_t xdr_proc;
  caddr_t xdr_where;

  xdrs->x_op = XDR_ENCODE;
  XDR_SETPOS(xdrs, 0);
  msg->rm_xid = su->su_xid;

  if(msg->rm_reply.rp_stat == MSG_ACCEPTED && msg->rm_reply.rp_acpt.ar_stat == SUCCESS)
    {
      xdr_proc = msg->acpted_rply.ar_results.proc;
      xdr_where = msg->acpted_rply.ar_results.where;
      msg->acpted_rply.ar_results.proc = (xdrproc_t) xdr_void;
      msg->acpted_rply.ar_results.where = NULL;

      if(!xdr_replymsg(xdrs, msg) ||
         !SVCAUTH_WRAP(xprt->xp_auth, xdrs, xdr_proc, xdr_where))
        return (FALSE);
    }
  else if(!xdr_replymsg(xdrs, msg))
    {
      return (FALSE);
    }
  slen = (int)XDR_GETPOS(xdrs);

#ifdef _FREEBSD
  if(sendto(xprt->xp_fd,
#else
  if(sendto(xprt->xp_sock,
#endif
            rpc_buffer(xprt),
            slen, 0, (struct sockaddr *)&(xprt->xp_raddr), xprt->xp_addrlen) != slen)
    {
      return (FALSE);
    }
  return (TRUE);
}

static bool_t Svcudp_getargs(SVCXPRT * xprt, xdrproc_t xdr_args, caddr_t args_ptr)
{
  return (SVCAUTH_UNWRAP(xprt->xp_auth, &(Su_data(xprt)->su_xdrs), xdr_args, args_ptr));
}

static bool_t Svcudp_freeargs(SVCXPRT * xprt, xdrproc_t xdr_args, caddr_t args_ptr)
{
  register XDR *xdrs = &(Su_data(xprt)->su_xdrs);

  xdrs->x_op = XDR_FREE;
  return ((*xdr_args) (xdrs, args_ptr));
}
