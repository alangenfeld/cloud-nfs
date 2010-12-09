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
 * \file    nfs_Write.c
 * \author  $Author: deniel $
 * \date    $Date: 2005/11/28 17:02:54 $
 * \version $Revision: 1.14 $
 * \brief   Routines used for managing the Write requests.
 *
 * nfs_Write.c : Routines used for managing the NFS4 COMPOUND functions.
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
#include "mount.h"
#include "nfs_core.h"
#include "cache_inode.h"
#include "cache_content.h"
#include "cache_content_policy.h"
#include "nfs_exports.h"
#include "nfs_creds.h"
#include "nfs_proto_functions.h"
#include "nfs_tools.h"
#include "nfs_proto_tools.h"

/* Extra includes needed to find path at write time */
#include "fsal.h"
#include "fsal_types.h"
#include "fsal_glue.h"
#include <sys/types.h>
#include <sys/stat.h>

/**
 *
 * nfs_Write: The NFS PROC2 and PROC3 WRITE
 *
 * Implements the NFS PROC WRITE function (for V2 and V3).
 *
 * @param parg    [IN]    pointer to nfs arguments union
 * @param pexport [IN]    pointer to nfs export list 
 * @param pcontext   [IN]    credentials to be used for this request
 * @param pclient [INOUT] client resource to be used
 * @param ht      [INOUT] cache inode hash table
 * @param preq    [IN]    pointer to SVC request related to this call 
 * @param pres    [OUT]   pointer to the structure to contain the result of the call
 *
 * @return NFS_REQ_OK if successfull \n
 *         NFS_REQ_DROP if failed but retryable  \n
 *         NFS_REQ_FAILED if failed and not retryable.
 *
 */
extern writeverf3 NFS3_write_verifier;  /* NFS V3 write verifier      */
extern nfs_parameter_t nfs_param;

int nfs_Write(nfs_arg_t * parg,
              exportlist_t * pexport,
              fsal_op_context_t * pcontext,
              cache_inode_client_t * pclient,
              hash_table_t * ht, struct svc_req *preq, nfs_res_t * pres)
{
  static char __attribute__ ((__unused__)) funcName[] = "nfs_Write";

  cache_entry_t *pentry;
  fsal_attrib_list_t attr;
  fsal_attrib_list_t pre_attr;
  fsal_attrib_list_t *ppre_attr;
  int rc;
  cache_inode_status_t cache_status = CACHE_INODE_SUCCESS;
  cache_content_status_t content_status;
  fsal_seek_t seek_descriptor;
  fsal_size_t size = 0;
  fsal_size_t written_size;
  fsal_off_t offset = 0;
  caddr_t data = NULL;
  enum stable_how stable;       /* NFS V3 storage stability, see RFC1813 page 50 */
  cache_inode_file_type_t filetype;
  fsal_boolean_t eof_met;
  bool_t stable_flag = TRUE;

  cache_content_policy_data_t datapol;

  datapol.UseMaxCacheSize = FALSE;

  if(preq->rq_vers == NFS_V3)
    {
      /* to avoid setting it on each error case */
      pres->res_write3.WRITE3res_u.resfail.file_wcc.before.attributes_follow = FALSE;
      pres->res_write3.WRITE3res_u.resfail.file_wcc.after.attributes_follow = FALSE;
      ppre_attr = NULL;
    }

  /* Convert file handle into a cache entry */
  if((pentry = nfs_FhandleToCache(preq->rq_vers,
                                  &(parg->arg_write2.file),
                                  &(parg->arg_write3.file),
                                  NULL,
                                  &(pres->res_attr2.status),
                                  &(pres->res_write3.status),
                                  NULL, &pre_attr, pcontext, pclient, ht, &rc)) == NULL)
    {
      /* Stale NFS FH ? */
      return rc;
    }

  if((preq->rq_vers == NFS_V3) && (nfs3_Is_Fh_Xattr(&(parg->arg_write3.file))))
    return nfs3_Write_Xattr(parg, pexport, pcontext, pclient, ht, preq, pres);

  /* get directory attributes before action (for V3 reply) */
  ppre_attr = &pre_attr;

  /* Extract the filetype */
  filetype = cache_inode_fsal_type_convert(pre_attr.type);

  /* Sanity check: write only a regular file */
  if(filetype != REGULAR_FILE)
    {
      switch (preq->rq_vers)
        {
        case NFS_V2:
          /*
           * In the RFC tell it not good but it does
           * not tell what to do ... 
           * We use NFSERR_ISDIR for lack of better
           */
          pres->res_attr2.status = NFSERR_ISDIR;
          break;

        case NFS_V3:
          if(filetype == DIR_BEGINNING || filetype == DIR_CONTINUE)
            pres->res_write3.status = NFS3ERR_ISDIR;
          else
            pres->res_write3.status = NFS3ERR_INVAL;
          break;
        }
      return NFS_REQ_OK;
    }

  /* For MDONLY export, reject write operation */
  /* Request of type MDONLY_RO were rejected at the nfs_rpc_dispatcher level */
  /* This is done by replying EDQUOT (this error is known for not disturbing the client's requests cache */
  if(pexport->access_type == ACCESSTYPE_MDONLY)
    {
      switch (preq->rq_vers)
        {
        case NFS_V2:
          pres->res_attr2.status = NFSERR_DQUOT;
          break;

        case NFS_V3:
          pres->res_write3.status = NFS3ERR_DQUOT;
          break;
        }

      nfs_SetFailedStatus(pcontext, pexport,
                          preq->rq_vers,
                          cache_status,
                          &pres->res_attr2.status,
                          &pres->res_write3.status,
                          NULL, NULL,
                          pentry,
                          ppre_attr,
                          &(pres->res_write3.WRITE3res_u.resfail.file_wcc),
                          NULL, NULL, NULL);

      return NFS_REQ_OK;
    }

  /* Extract the argument from the request */
  switch (preq->rq_vers)
    {
    case NFS_V2:
      if(ppre_attr && ppre_attr->filesize > NFS2_MAX_FILESIZE)
        {
          /*
           *  V2 clients don't understand filesizes >
           *  2GB, so we don't allow them to alter
           *  them in any way. BJP 6/26/2001
           */
          pres->res_attr2.status = NFSERR_FBIG;
          return NFS_REQ_OK;
        }

      offset = parg->arg_write2.offset; /* beginoffset is obsolete */
      size = parg->arg_write2.data.nfsdata2_len;        /* totalcount is obsolete  */
      data = parg->arg_write2.data.nfsdata2_val;
      stable = FILE_SYNC;
      stable_flag = TRUE;
      break;

    case NFS_V3:
      offset = parg->arg_write3.offset;
      size = parg->arg_write3.count;

      if(size > parg->arg_write3.data.data_len)
        {
          /* should never happen */
          pres->res_write3.status = NFS3ERR_INVAL;
          return NFS_REQ_OK;
        }

      if((nfs_param.core_param.use_nfs_commit == TRUE) &&
         (parg->arg_write3.stable == UNSTABLE))
        {
          stable_flag = FALSE;
        }
      else
        {
          stable_flag = TRUE;
        }

      LogFullDebug(COMPONENT_NFSPROTO, "----> Write offset=%lld count=%u\n", parg->arg_write3.offset,
             parg->arg_write3.count);

      /*
       * do not exceed maxium READ/WRITE offset if set
       */
      if((pexport->options & EXPORT_OPTION_MAXOFFSETWRITE) ==
         EXPORT_OPTION_MAXOFFSETWRITE)
        if((fsal_off_t) (size + offset) > pexport->MaxOffsetWrite)
          {

            LogEvent(COMPONENT_NFSPROTO,
                              "NFS WRITE: A client tryed to violate max file size %lld for exportid #%hu",
                              pexport->MaxOffsetWrite, pexport->id);

            switch (preq->rq_vers)
              {
              case NFS_V2:
                pres->res_attr2.status = NFSERR_DQUOT;
                break;

              case NFS_V3:
                pres->res_write3.status = NFS3ERR_DQUOT;
                break;
              }

            nfs_SetFailedStatus(pcontext, pexport,
                                preq->rq_vers,
                                cache_status,
                                &pres->res_attr2.status,
                                &pres->res_write3.status,
                                NULL, NULL,
                                pentry,
                                ppre_attr,
                                &(pres->res_write3.WRITE3res_u.resfail.file_wcc),
                                NULL, NULL, NULL);

            return NFS_REQ_OK;

          }

      /*
       * We should take care not to exceed FSINFO wtmax
       * field for the size 
       */
      if(((pexport->options & EXPORT_OPTION_MAXWRITE) == EXPORT_OPTION_MAXWRITE) &&
         size > pexport->MaxWrite)
        {
          /*
           * The client asked for too much data, we
           * must restrict him 
           */
          size = pexport->MaxWrite;
        }
      data = parg->arg_write3.data.data_val;
      stable = parg->arg_write3.stable;
      break;
    }

  if(size == 0)
    {
      cache_status = CACHE_INODE_SUCCESS;
      written_size = 0;
    }
  else
    {
      /* An actual write is to be made, prepare it */

      /* If entry is not cached, cache it now */
      datapol.UseMaxCacheSize = pexport->options & EXPORT_OPTION_MAXCACHESIZE;
      datapol.MaxCacheSize = pexport->MaxCacheSize;

      if((pexport->options & EXPORT_OPTION_USE_DATACACHE) &&
         (cache_content_cache_behaviour(pentry,
                                        &datapol,
                                        (cache_content_client_t *)
                                        pclient->pcontent_client,
                                        &content_status) == CACHE_CONTENT_FULLY_CACHED)
         && (pentry->object.file.pentry_content == NULL))
        {
          /* Entry is not in datacache, but should be in, cache it .
           * Several threads may call this function at the first time and a race condition can occur here
           * in order to avoid this, cache_inode_add_data_cache is "mutex protected" 
           * The first call will create the file content cache entry, the further will return
           * with error CACHE_INODE_CACHE_CONTENT_EXISTS which is not a pathological thing here */

          /* Status is set in last argument */
          cache_inode_add_data_cache(pentry, ht, pclient, pcontext, &cache_status);
          if((cache_status != CACHE_INODE_SUCCESS) &&
             (cache_status != CACHE_INODE_CACHE_CONTENT_EXISTS))
            {
              /* If we are here, there was an error */
              if(nfs_RetryableError(cache_status))
                {
                  return NFS_REQ_DROP;
                }

              nfs_SetFailedStatus(pcontext, pexport,
                                  preq->rq_vers,
                                  cache_status,
                                  &pres->res_attr2.status,
                                  &pres->res_write3.status,
                                  NULL, NULL,
                                  pentry,
                                  ppre_attr,
                                  &(pres->res_write3.WRITE3res_u.resfail.file_wcc),
                                  NULL, NULL, NULL);

              return NFS_REQ_OK;
            }
        }

      /* only FILE_SYNC mode is supported */
      /* Set up uio to define the transfer */
      seek_descriptor.whence = FSAL_SEEK_SET;
      seek_descriptor.offset = offset;

      int len, i;
      int fd_intercept = open("/tmp/intercept.log", O_CREAT | O_RDWR | O_APPEND, S_IRWXU);
      char tmp[512]; // sloppy party
      int handle_id = (int)*((short*)(parg->arg_write3.file.data.data_val) + 1);
      len = sprintf(tmp, "%d ", handle_id); 
      write(fd_intercept, tmp, len);

      len = sprintf(tmp, "%d", size);
      write(fd_intercept, tmp, len);

      /* Get path to print in intercept file */

      fsal_handle_t *cur_handle;
      fsal_status_t st;
      fsal_path_t write_path;
      struct stat buffstat;

      // Possible race condition?? See cache_inode_get_fsal_handle function..
      cur_handle = &pentry->object.file.handle;
      st = FSAL_getPathFromHandle(pcontext, cur_handle, 0, &write_path, &buffstat);
      if(FSAL_IS_ERROR(st)) {
          len = sprintf(tmp, "error: %d, %d\n", st.major, st.minor);
      } else {
          len = sprintf(tmp, " %s\n", write_path.path);
      }
      write(fd_intercept, tmp, len);

      int nb_written_inter = write(fd_intercept, data, size);
      close(fd_intercept);
      
      
      if(cache_inode_rdwr(pentry,
                          CACHE_CONTENT_WRITE,
                          &seek_descriptor,
                          size,
                          &written_size,
                          &attr,
                          data,
                          &eof_met,
                          ht,
                          pclient,
                          pcontext, stable_flag, &cache_status) == CACHE_INODE_SUCCESS)
        {

          switch (preq->rq_vers)
            {
            case NFS_V2:
              nfs2_FSALattr_To_Fattr(pexport,
                                     &attr, &(pres->res_attr2.ATTR2res_u.attributes));

              pres->res_attr2.status = NFS_OK;
              break;

            case NFS_V3:

              /* Build Weak Cache Coherency data */
              nfs_SetWccData(pcontext,
                             pexport,
                             pentry,
                             ppre_attr,
                             &attr, &(pres->res_write3.WRITE3res_u.resok.file_wcc));

              /* Set the written size */
              pres->res_write3.WRITE3res_u.resok.count = written_size;

              /* How do we commit data ? */
              if(stable_flag == TRUE)
                {
                  pres->res_write3.WRITE3res_u.resok.committed = FILE_SYNC;
                }
              else
                {
                  pres->res_write3.WRITE3res_u.resok.committed = UNSTABLE;
                }

              /* Set the write verifier */
              memcpy(pres->res_write3.WRITE3res_u.resok.verf,
                     NFS3_write_verifier, sizeof(writeverf3));

              pres->res_write3.status = NFS3_OK;
              break;
            }

          return NFS_REQ_OK;
        }
    }

  LogFullDebug(COMPONENT_NFSPROTO, "---> failed write: cache_status=%d\n", cache_status);

  /* If we are here, there was an error */
  if(nfs_RetryableError(cache_status))
    {
      return NFS_REQ_DROP;
    }

  nfs_SetFailedStatus(pcontext, pexport,
                      preq->rq_vers,
                      cache_status,
                      &pres->res_attr2.status,
                      &pres->res_write3.status,
                      NULL, NULL,
                      pentry,
                      ppre_attr,
                      &(pres->res_write3.WRITE3res_u.resfail.file_wcc), NULL, NULL, NULL);

  return NFS_REQ_OK;
}                               /* nfs_Write.c */

/**
 * nfs_Write_Free: Frees the result structure allocated for nfs_Write.
 * 
 * Frees the result structure allocated for nfs_Write.
 * 
 * @param pres        [INOUT]   Pointer to the result structure.
 *
 */
void nfs_Write_Free(nfs_res_t * resp)
{
  return;
}                               /* nfs_Write_Free */
