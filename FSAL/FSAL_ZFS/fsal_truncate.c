/*
 * vim:expandtab:shiftwidth=8:tabstop=8:
 */

/**
 *
 * \file    fsal_truncate.c
 * \author  $Author: leibovic $
 * \date    $Date: 2005/07/29 09:39:05 $
 * \version $Revision: 1.4 $
 * \brief   Truncate function.
 *
 */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "fsal.h"
#include "fsal_internal.h"
#include "fsal_convert.h"

/**
 * FSAL_truncate:
 * Modify the data length of a regular file.
 *
 * \param filehandle (input):
 *        Handle of the file is to be truncated.
 * \param cred (input):
 *        Authentication context for the operation (user,...).
 * \param length (input):
 *        The new data length for the file.
 * \param object_attributes (optionnal input/output): 
 *        The post operation attributes of the file.
 *        As input, it defines the attributes that the caller
 *        wants to retrieve (by positioning flags into this structure)
 *        and the output is built considering this input
 *        (it fills the structure according to the flags it contains).
 *        May be NULL.
 *
 * \return Major error codes :
 *        - ERR_FSAL_NO_ERROR     (no error)
 *        - ERR_FSAL_STALE        (filehandle does not address an existing object)
 *        - ERR_FSAL_INVAL        (filehandle does not address a regular file)
 *        - ERR_FSAL_FAULT        (a NULL pointer was passed as mandatory argument)
 *        - Other error codes can be returned :
 *          ERR_FSAL_ACCESS, ERR_FSAL_IO, ...
 */

fsal_status_t ZFSFSAL_truncate(zfsfsal_handle_t * filehandle, /* IN */
                            zfsfsal_op_context_t * p_context,      /* IN */
                            fsal_size_t length, /* IN */
                            zfsfsal_file_t * file_descriptor,      /* Unused in this FSAL */
                            fsal_attrib_list_t * object_attributes      /* [ IN/OUT ] */
    )
{
  int rc;

  /* sanity checks.
   * note : object_attributes is optional.
   */
  if(!filehandle || !p_context)
    Return(ERR_FSAL_FAULT, 0, INDEX_FSAL_truncate);

  /* >> check object type if it's stored into the filehandle << */
  if(filehandle->data.type != FSAL_TYPE_FILE)
      Return(ERR_FSAL_INVAL, 0, INDEX_FSAL_truncate);

  TakeTokenFSCall();

  rc = libzfswrap_truncate(p_context->export_context->p_vfs, &p_context->user_credential.cred,
                           filehandle->data.zfs_handle, length);

  ReleaseTokenFSCall();

  /* >> interpret error code << */
  if(rc)
    Return(posix2fsal_error(rc), 0, INDEX_FSAL_truncate);

  /* >> Optionnaly retrieve post op attributes
   * If your filesystem truncate call can't return them,
   * you can proceed like this : <<
   */
  if(object_attributes)
    {

      fsal_status_t st;

      st = ZFSFSAL_getattrs(filehandle, p_context, object_attributes);

      if(FSAL_IS_ERROR(st))
        {
          FSAL_CLEAR_MASK(object_attributes->asked_attributes);
          FSAL_SET_MASK(object_attributes->asked_attributes, FSAL_ATTR_RDATTR_ERR);
        }

    }

  /* No error occured */
  Return(ERR_FSAL_NO_ERROR, 0, INDEX_FSAL_truncate);

}
