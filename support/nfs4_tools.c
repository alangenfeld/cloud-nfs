/*
 * vim:expandtab:shiftwidth=8:tabstop=8:
 *
 * Copyright CEA/DAM/DIF  (2008)
 * contributeur : Philippe DENIEL   philippe.deniel@cea.fr
 *                Thomas LEIBOVICI  thomas.leibovici@cea.fr
 *
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
 * ---------------------------------------
 */

/**
 * \file    nfs4_tools.c
 * \author  $Author: leibovic $
 * \date    $Date: 2006/01/20 07:39:22 $
 * \version $Revision: 1.43 $
 * \brief   Some tools very usefull in the nfs4 protocol implementation.
 *
 * nfs_tools.c : Some tools very usefull in the nfs protocol implementation
 *
 * $Header: /cea/home/cvs/cvs/SHERPA/BaseCvs/GANESHA/src/MainNFSD/nfs_tools.c,v 1.43 2006/01/20 07:39:22 leibovic Exp $
 *
 * $Log$
 *
 */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef _SOLARIS
#include "solaris_port.h"
#endif

#include <stdio.h>
#include <sys/types.h>
#include <ctype.h>              /* for having isalnum */
#include <stdlib.h>             /* for having atoi */
#include <dirent.h>             /* for having MAXNAMLEN */
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <pthread.h>
#include <fcntl.h>
#include <sys/file.h>           /* for having FNDELAY */
#include <pwd.h>
#include <grp.h>
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
#include "HashData.h"
#include "HashTable.h"
#include "nfs_core.h"
#include "nfs23.h"
#include "nfs4.h"
#include "fsal.h"
#include "nfs_tools.h"
#include "nfs_exports.h"
#include "nfs_file_handle.h"

extern time_t ServerBootTime;
extern nfs_parameter_t nfs_param;

/**
 *
 * nfs4_is_leased_expired
 *
 * This routine checks the availability of a lease to a cache_entry
 *
 * @param pstate [IN] pointer to the cache_entry to be checked.
 *
 * @return 1 if expired, 0 otherwise.
 *
 */
int nfs4_is_lease_expired(cache_entry_t * pentry)
{
  nfs_client_id_t nfs_clientid;

  if(pentry->internal_md.type != REGULAR_FILE)
    return 0;

#ifdef BUGAZOMEU
  if(pentry->object.file.state_v4 == NULL)
    return 0;

  if(nfs_client_id_get(pentry->object.file.state_v4->clientid4, &nfs_clientid) !=
     CLIENT_ID_SUCCESS)
    return 0;                   /* No client id, manage it as non-expired */

  LogFullDebug(COMPONENT_NFS_V4, "Lease on %p for client_name = %s id=%lld\n", pentry, nfs_clientid.client_name,
         nfs_clientid.clientid);

  LogFullDebug(COMPONENT_NFS_V4, "--------- nfs4_is_lease_expired ---------> %u %u delta=%u lease=%u\n",
         time(NULL), time(NULL), nfs_clientid.last_renew,
         time(NULL) - nfs_clientid.last_renew, nfs_param.nfsv4_param.lease_lifetime);
#endif

  /* Check is lease is still valid */
  if(time(NULL) - nfs_clientid.last_renew > (int)nfs_param.nfsv4_param.lease_lifetime)
    return 1;
  else
    return 0;
}                               /* nfs4_is_lease_expired */
