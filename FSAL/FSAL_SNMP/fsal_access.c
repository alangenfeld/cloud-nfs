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
#endif

#include "fsal.h"
#include "fsal_internal.h"
#include "fsal_convert.h"

/**
 * FSAL_access :
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
fsal_status_t SNMPFSAL_access(snmpfsal_handle_t * object_handle,        /* IN */
                              snmpfsal_op_context_t * p_context,        /* IN */
                              fsal_accessflags_t access_type,   /* IN */
                              fsal_attrib_list_t * object_attributes    /* [ IN/OUT ] */
    )
{
  fsal_attrib_list_t attrs;
  fsal_status_t st;

  /* sanity checks.
   * note : object_attributes is optional in FSAL_access.
   */
  if(!object_handle || !p_context)
    Return(ERR_FSAL_FAULT, 0, INDEX_FSAL_access);

  FSAL_CLEAR_MASK(attrs.asked_attributes);
  FSAL_SET_MASK(attrs.asked_attributes, global_fs_info.supported_attrs);

  st = SNMPFSAL_getattrs(object_handle, p_context, &attrs);

  if(FSAL_IS_ERROR(st))
    Return(st.major, st.minor, INDEX_FSAL_access);

  /* set attributes if needed, then call test_access.
   */
  if(object_attributes)
    {
      *object_attributes = attrs;
    }

  st = SNMPFSAL_test_access(p_context, access_type, &attrs);

  Return(st.major, st.minor, INDEX_FSAL_access);

}
