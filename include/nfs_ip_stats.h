#ifndef _NFS_IP_STATS_H
#define _NFS_IP_STATS_H

#include <sys/types.h>
#include <sys/param.h>

#ifdef _USE_GSSRPC
#include <gssapi/gssapi.h>
#include <gssapi/gssapi_krb5.h>
#include <gssrpc/rpc.h>
#include <gssrpc/rpc.h>
#include <gssrpc/svc.h>
#else
#include <rpc/rpc.h>
#include <rpc/types.h>
#include <rpc/svc.h>
#endif

#include <dirent.h>             /* for having MAXNAMLEN */
#include <netdb.h>              /* for having MAXHOSTNAMELEN */
#include "HashData.h"
#include "HashTable.h"

/* IP/name cache error */
#define IP_NAME_SUCCESS             0
#define IP_NAME_INSERT_MALLOC_ERROR 1
#define IP_NAME_NOT_FOUND           2
#define IP_NAME_NETDB_ERROR         3

/* IP/stats cache error */
#define IP_STATS_SUCCESS             0
#define IP_STATS_INSERT_MALLOC_ERROR 1
#define IP_STATS_NOT_FOUND           2
#define IP_STATS_NETDB_ERROR         3

#define IP_NAME_PREALLOC_SIZE      200

/* NFS IPaddr cache entry structure */
typedef struct nfs_ip_name__
{
  unsigned int ipaddr;
  time_t timestamp;
  struct nfs_ip_name__ *next_alloc;
  char hostname[MAXHOSTNAMELEN];
} nfs_ip_name_t;

typedef struct nfs_ip_stats__
{
  unsigned int nb_call;
  unsigned int nb_req_nfs2;
  unsigned int nb_req_nfs3;
  unsigned int nb_req_nfs4;
  unsigned int nb_req_mnt1;
  unsigned int nb_req_mnt3;
  unsigned int req_mnt1[MNT_V1_NB_COMMAND];
  unsigned int req_mnt3[MNT_V3_NB_COMMAND];
  unsigned int req_nfs2[NFS_V2_NB_COMMAND];
  unsigned int req_nfs3[NFS_V3_NB_COMMAND];

  struct nfs_ip_stats__ *next_alloc;
} nfs_ip_stats_t;

int nfs_ip_name_get(unsigned int ipaddr, char *hostname);
int nfs_ip_name_add(unsigned int ipaddr, char *hostname);
int nfs_ip_name_remove(int ipaddr);

int nfs_ip_stats_add(hash_table_t * ht_ip_stats,
                     unsigned int ipaddr, nfs_ip_stats_t * nfs_ip_stats_pool);

int nfs_ip_stats_incr(hash_table_t * ht_ip_stats,
                      unsigned int ipaddr,
                      unsigned int nfs_prog,
                      unsigned int mnt_prog, struct svc_req *ptr_req);

int nfs_ip_stats_get(hash_table_t * ht_ip_stats,
                     unsigned int ipaddr, nfs_ip_stats_t ** pnfs_ip_stats);

int nfs_ip_stats_remove(hash_table_t * ht_ip_stats,
                        int ipaddr, nfs_ip_stats_t * nfs_ip_stats_pool);
void nfs_ip_stats_dump(hash_table_t ** ht_ip_stats,
                       unsigned int nb_worker, char *path_stat);

void nfs_ip_name_get_stats(hash_stat_t * phstat);
int nfs_ip_name_populate(char *path);

int display_ip_name(hash_buffer_t * pbuff, char *str);
int display_ip_value(hash_buffer_t * pbuff, char *str);
int compare_ip_name(hash_buffer_t * buff1, hash_buffer_t * buff2);
unsigned long int ip_name_rbt_hash_func(hash_parameter_t * p_hparam,
                                        hash_buffer_t * buffclef);
unsigned long int ip_name_value_hash_func(hash_parameter_t * p_hparam,
                                          hash_buffer_t * buffclef);

int display_ip_stats(hash_buffer_t * pbuff, char *str);
int compare_ip_stats(hash_buffer_t * buff1, hash_buffer_t * buff2);
unsigned long int ip_stats_rbt_hash_func(hash_parameter_t * p_hparam,
                                         hash_buffer_t * buffclef);
unsigned long int ip_stats_value_hash_func(hash_parameter_t * p_hparam,
                                           hash_buffer_t * buffclef);

#endif
