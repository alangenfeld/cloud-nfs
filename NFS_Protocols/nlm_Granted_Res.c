/*
 * Copyright IBM Corporation, 2010
 *  Contributor: M. Mohan Kumar <mohan@in.ibm.com>
 *
 * --------------------------
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef _SOLARIS
#include "solaris_port.h"
#endif

#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <fcntl.h>
#include <sys/file.h>           /* for having FNDELAY */
#include "HashData.h"
#include "HashTable.h"
#ifdef _USE_GSSRPC
#include <gssrpc/types.h>
#include <gssrpc/rpc.h>
#include <gssrpc/auth.h>
#include <gssrpc/pmap_clnt.h>
#else
#include <rpc/types.h>
#include <rpc/rpc.h>
#include <rpc/auth.h>
#include <rpc/pmap_clnt.h>
#endif
#include "log_macros.h"
#include "stuff_alloc.h"
#include "nfs23.h"
#include "nfs4.h"
#include "nfs_core.h"
#include "cache_inode.h"
#include "cache_content.h"
#include "nfs_exports.h"
#include "nfs_creds.h"
#include "nfs_tools.h"
#include "mount.h"
#include "nfs_proto_functions.h"
#include "nlm_util.h"
#include "nlm4.h"
#include "nlm_async.h"

/**
 * nlm4_Granted_Res: Lock Granted Result Handler
 *
 *  @param parg        [IN]
 *  @param pexportlist [IN]
 *  @param pcontextp   [IN]
 *  @param pclient     [INOUT]
 *  @param ht          [INOUT]
 *  @param preq        [IN]
 *  @param pres        [OUT]
 *
 */
int nlm4_Granted_Res(nfs_arg_t * parg /* IN     */ ,
                     exportlist_t * pexport /* IN     */ ,
                     fsal_op_context_t * pcontext /* IN     */ ,
                     cache_inode_client_t * pclient /* INOUT  */ ,
                     hash_table_t * ht /* INOUT  */ ,
                     struct svc_req *preq /* IN     */ ,
                     nfs_res_t * pres /* OUT    */ )
{
  nlm4_res *arg;
  nlm_lock_entry_t *nlm_entry;

  LogFullDebug(COMPONENT_NFSPROTO,
                    "REQUEST PROCESSING: Calling nlm_Granted_Res");

  arg = &parg->arg_nlm4_res;
  nlm_entry = nlm_find_lock_entry_by_cookie(&arg->cookie);
  if(!nlm_entry)
    return NFS_REQ_OK;

  if(arg->stat.stat == NLM4_DENIED_GRACE_PERIOD)
    {
      LogMajor(COMPONENT_NFSPROTO, "%s: Granted call failed due to client grace period\n");
      LogMajor(COMPONENT_NFSPROTO, "Retrying...\n");

      /*
       * nlm_resend_grant_msg will drop the lock entry ref count
       */
      nlm_async_callback(nlm_resend_grant_msg, (void *)nlm_entry);
    }
  else
    nlm_lock_entry_dec_ref(nlm_entry);
  /*
   * Consider all other return status as success
   * nlm_entry is already marked NLM4_GRANTED
   */
  return NFS_REQ_OK;
}

/**
 * nlm4_Granted_Res_Free: Frees the result structure allocated for
 * nlm4_Granted_Res
 *
 * Frees the result structure allocated for nlm4_Granted_Res. Does Nothing in fact.
 *
 * @param pres        [INOUT]   Pointer to the result structure.
 *
 */
void nlm4_Granted_Res_Free(nfs_res_t * pres)
{
  return;
}
