/*
 * vim:expandtab:shiftwidth=8:tabstop=8:
 */

/**
 *
 * \file    fsal_access.c
 * \author  $Author: leibovic $
 * \date    $Date: 2006/01/17 14:20:07 $
 * \version $Revision: 1.16 $
 * \brief   FSAL access permissions functions.
 *
 */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef _SOLARIS
#include "solaris_port.h"
#endif                          /* _SOLARIS */

#ifdef _USE_GSSRPC
#include <gssrpc/rpc.h>
#include <gssrpc/xdr.h>
#else
#include <rpc/rpc.h>
#include <rpc/xdr.h>
#endif
#include "nfs4.h"

#include <string.h>

#include "stuff_alloc.h"
#include "fsal.h"
#include "fsal_types.h"
#include "fsal_internal.h"
#include "fsal_convert.h"
#include "fsal_common.h"

#include "nfs_proto_functions.h"
#include "fsal_nfsv4_macros.h"

/**
 * PROXYFSAL_access :
 * Tests whether the user or entity identified by the p_context structure
 * can access the object identified by object_handle,
 * as indicated by the access_type parameter.
 *
 * \param object_handle (input):
 *        The handle of the object to test permissions on.
 * \param p_context (input):
 *        Authentication context for the operation (export entry, user,...).
 * \param access_type (input):
 *        Indicates the permissions to be tested.
 *        This is an inclusive OR of the permissions
 *        to be checked for the user specified by p_context.
 *        Permissions constants are :
 *        - FSAL_R_OK : test for read permission
 *        - FSAL_W_OK : test for write permission
 *        - FSAL_X_OK : test for exec permission
 *        - FSAL_F_OK : test for file existence
 * \param object_attributes (optional input/output):
 *        The post operation attributes for the object.
 *        As input, it defines the attributes that the caller
 *        wants to retrieve (by positioning flags into this structure)
 *        and the output is built considering this input
 *        (it fills the structure according to the flags it contains).
 *        Can be NULL.
 *
 * \return Major error codes :
 *        - ERR_FSAL_NO_ERROR     (no error, asked permission is granted)
 *        - ERR_FSAL_ACCESS       (object permissions doesn't fit asked access type)
 *        - ERR_FSAL_STALE        (object_handle does not address an existing object)
 *        - ERR_FSAL_FAULT        (a NULL pointer was passed as mandatory argument)
 *        - Other error codes when something anormal occurs.
 */
fsal_status_t PROXYFSAL_access(proxyfsal_handle_t * object_handle,      /* IN */
                               proxyfsal_op_context_t * p_context,      /* IN */
                               fsal_accessflags_t access_type,  /* IN */
                               fsal_attrib_list_t * object_attributes   /* [ IN/OUT ] */
    )
{

  int rc;
  COMPOUND4args argnfs4;
  COMPOUND4res resnfs4;
  bitmap4 bitmap;
  uint32_t bitmap_val[2];
  uint32_t bitmap_res[2];
  uint32_t accessflag = 0;

  fsal_proxy_internal_fattr_t fattr_internal;

  struct timeval __attribute__ ((__unused__)) timeout = {25, 0};
  nfs_fh4 nfs4fh;

#define FSAL_ACCESS_NB_OP_ACCESS 3

  nfs_argop4 argoparray[FSAL_ACCESS_NB_OP_ACCESS];
  nfs_resop4 resoparray[FSAL_ACCESS_NB_OP_ACCESS];

  /* sanity checks.
   * note : object_attributes is optional in PROXYFSAL_access.
   */
  if(!object_handle || !p_context)
    Return(ERR_FSAL_FAULT, 0, INDEX_FSAL_access);

  /* Setup results structures */
  argnfs4.argarray.argarray_val = argoparray;
  resnfs4.resarray.resarray_val = resoparray;
  fsal_internal_proxy_setup_fattr(&fattr_internal);
  argnfs4.minorversion = 0;
  /* argnfs4.tag.utf8string_val = "GANESHA NFSv4 Proxy: Access" ; */
  argnfs4.tag.utf8string_val = NULL;
  argnfs4.tag.utf8string_len = 0;
  argnfs4.argarray.argarray_len = 0;

  bitmap.bitmap4_val = bitmap_val;
  bitmap.bitmap4_len = 2;
  fsal_internal_proxy_create_fattr_bitmap(&bitmap);

  /* Set up access flags */
  if(access_type & FSAL_R_OK)
    accessflag |= ACCESS4_READ;

  if(access_type & FSAL_X_OK)
    accessflag |= ACCESS4_LOOKUP;

  if(access_type & FSAL_W_OK)
    accessflag |= (ACCESS4_MODIFY & ACCESS4_EXTEND);

  if(access_type & FSAL_F_OK)
    accessflag |= ACCESS4_LOOKUP;

  /* >> convert your fsal access type to your FS access type << */

  /* Get NFSv4 File handle */
  if(fsal_internal_proxy_extract_fh(&nfs4fh, object_handle) == FALSE)
    Return(ERR_FSAL_FAULT, 0, INDEX_FSAL_access);

#define FSAL_ACCESS_IDX_OP_PUTFH    0
#define FSAL_ACCESS_IDX_OP_ACCESS   1
#define FSAL_ACCESS_IDX_OP_GETATTR  2

  COMPOUNDV4_ARG_ADD_OP_PUTFH(argnfs4, nfs4fh);
  COMPOUNDV4_ARG_ADD_OP_ACCESS(argnfs4, accessflag);
  COMPOUNDV4_ARG_ADD_OP_GETATTR(argnfs4, bitmap);

  resnfs4.resarray.resarray_val[FSAL_ACCESS_IDX_OP_GETATTR].nfs_resop4_u.opgetattr.
      GETATTR4res_u.resok4.obj_attributes.attrmask.bitmap4_val = bitmap_res;
  resnfs4.resarray.resarray_val[FSAL_ACCESS_IDX_OP_GETATTR].nfs_resop4_u.opgetattr.
      GETATTR4res_u.resok4.obj_attributes.attrmask.bitmap4_len = 2;

  resnfs4.resarray.resarray_val[FSAL_ACCESS_IDX_OP_GETATTR].nfs_resop4_u.opgetattr.
      GETATTR4res_u.resok4.obj_attributes.attr_vals.attrlist4_val =
      (char *)&fattr_internal;
  resnfs4.resarray.resarray_val[FSAL_ACCESS_IDX_OP_GETATTR].nfs_resop4_u.opgetattr.
      GETATTR4res_u.resok4.obj_attributes.attr_vals.attrlist4_len =
      sizeof(fattr_internal);

  TakeTokenFSCall();

  /* Call the NFSv4 function */
  COMPOUNDV4_EXECUTE(p_context, argnfs4, resnfs4, rc);
  if(rc != RPC_SUCCESS)
    {
      ReleaseTokenFSCall();

      Return(ERR_FSAL_IO, resnfs4.status, INDEX_FSAL_access);
    }

  ReleaseTokenFSCall();

  /* >> convert the returned code, an return it on error << */
  if(resnfs4.status != NFS4_OK)
    return fsal_internal_proxy_error_convert(resnfs4.status, INDEX_FSAL_access);

  /* get attributes if object_attributes is not null.
   * If an error occures during getattr operation,
   * an error bit is set in the output structure.
   */
  if(object_attributes)
    {
      /* Use NFSv4 service function to build the FSAL_attr */
      if(nfs4_Fattr_To_FSAL_attr(object_attributes,
                                 &resnfs4.resarray.
                                 resarray_val[FSAL_ACCESS_IDX_OP_GETATTR].nfs_resop4_u.
                                 opgetattr.GETATTR4res_u.resok4.obj_attributes) != 1)
        {
          FSAL_CLEAR_MASK(object_attributes->asked_attributes);
          FSAL_SET_MASK(object_attributes->asked_attributes, FSAL_ATTR_RDATTR_ERR);

          Return(ERR_FSAL_INVAL, 0, INDEX_FSAL_access);
        }

    }

  Return(ERR_FSAL_NO_ERROR, 0, INDEX_FSAL_access);

}
