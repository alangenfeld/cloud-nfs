/*
 * vim:expandtab:shiftwidth=8:tabstop=8:
 */

/**
 *
 * \file    fsal_attrs.c
 * \author  $Author: leibovic $
 * \date    $Date: 2005/09/09 15:22:49 $
 * \version $Revision: 1.19 $
 * \brief   Attributes functions.
 *
 */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "fsal.h"
#include "fsal_internal.h"
#include "fsal_convert.h"

#include <fcntl.h>

/**
 * ZFSFSAL_getattrs:
 * Get attributes for the object specified by its filehandle.
 *
 * \param filehandle (input):
 *        The handle of the object to get parameters.
 * \param p_context (input):
 *        Authentication context for the operation (user, export...).
 * \param object_attributes (mandatory input/output):
 *        The retrieved attributes for the object.
 *        As input, it defines the attributes that the caller
 *        wants to retrieve (by positioning flags into this structure)
 *        and the output is built considering this input
 *        (it fills the structure according to the flags it contains).
 *
 * \return Major error codes :
 *        - ERR_FSAL_NO_ERROR     (no error)
 *        - ERR_FSAL_STALE        (object_handle does not address an existing object)
 *        - ERR_FSAL_FAULT        (a NULL pointer was passed as mandatory argument) 
 *        - Another error code if an error occured.
 */
fsal_status_t ZFSFSAL_getattrs(zfsfsal_handle_t * filehandle, /* IN */
                               zfsfsal_op_context_t * p_context,      /* IN */
                               fsal_attrib_list_t * object_attributes      /* IN/OUT */
    )
{
  int rc, type;
  fsal_status_t status;
  struct stat fstat;

  /* sanity checks.
   * note : object_attributes is mandatory in ZFSFSAL_getattrs.
   */
  if(!filehandle || !p_context || !object_attributes)
    Return(ERR_FSAL_FAULT, 0, INDEX_FSAL_getattrs);

  TakeTokenFSCall();

  rc = libzfswrap_getattr(p_context->export_context->p_vfs, &p_context->user_credential.cred,
                          filehandle->data.zfs_handle, &fstat, &type);

  ReleaseTokenFSCall();

  /* >> convert error code, and return on error << */
  if(rc)
    Return(posix2fsal_error(rc), 0, INDEX_FSAL_getattrs);

  /* >> convert your filesystem attributes to FSAL attributes << */
  fsal_status_t st = posix2fsal_attributes(&fstat, object_attributes);
  if(FSAL_IS_ERROR(st))
    {
      FSAL_CLEAR_MASK(object_attributes->asked_attributes);
      FSAL_SET_MASK(object_attributes->asked_attributes, FSAL_ATTR_RDATTR_ERR);
      Return(st.major, st.minor, INDEX_FSAL_getattrs);
    }

  Return(ERR_FSAL_NO_ERROR, 0, INDEX_FSAL_getattrs);

}

/**
 * FSAL_setattrs:
 * Set attributes for the object specified by its filehandle.
 *
 * \param filehandle (input):
 *        The handle of the object to get parameters.
 * \param p_context (input):
 *        Authentication context for the operation (user,...).
 * \param attrib_set (mandatory input):
 *        The attributes to be set for the object.
 *        It defines the attributes that the caller
 *        wants to set and their values.
 * \param object_attributes (optionnal input/output):
 *        The post operation attributes for the object.
 *        As input, it defines the attributes that the caller
 *        wants to retrieve (by positioning flags into this structure)
 *        and the output is built considering this input
 *        (it fills the structure according to the flags it contains).
 *        May be NULL.
 *
 * \return Major error codes :
 *        - ERR_FSAL_NO_ERROR     (no error)
 *        - ERR_FSAL_STALE        (object_handle does not address an existing object)
 *        - ERR_FSAL_INVAL        (tried to modify a read-only attribute)
 *        - ERR_FSAL_ATTRNOTSUPP  (tried to modify a non-supported attribute)
 *        - ERR_FSAL_FAULT        (a NULL pointer was passed as mandatory argument)
 *        - Another error code if an error occured.
 *        NB: if getting postop attributes failed,
 *        the function does not return an error
 *        but the FSAL_ATTR_RDATTR_ERR bit is set in
 *        the object_attributes->asked_attributes field.
 */

fsal_status_t ZFSFSAL_setattrs(zfsfsal_handle_t * filehandle, /* IN */
                               zfsfsal_op_context_t * p_context,      /* IN */
                               fsal_attrib_list_t * attrib_set,    /* IN */
                               fsal_attrib_list_t * object_attributes      /* [ IN/OUT ] */
    )
{
  int rc;
  fsal_status_t status;
  fsal_attrib_list_t attrs;

  /* sanity checks.
   * note : object_attributes is optional.
   */
  if(!filehandle || !p_context || !attrib_set)
    Return(ERR_FSAL_FAULT, 0, INDEX_FSAL_setattrs);

  /* local copy of attributes */
  attrs = *attrib_set;

  /* First, check that FSAL attributes changes are allowed. */

  /* Is it allowed to change times ? */

  if(!global_fs_info.cansettime)
    {

      if(attrs.asked_attributes
         & (FSAL_ATTR_ATIME | FSAL_ATTR_CREATION | FSAL_ATTR_CTIME | FSAL_ATTR_MTIME))
        {

          /* handled as an unsettable attribute. */
          Return(ERR_FSAL_INVAL, 0, INDEX_FSAL_setattrs);
        }

    }

  /* apply umask, if mode attribute is to be changed */

  if(FSAL_TEST_MASK(attrs.asked_attributes, FSAL_ATTR_MODE))
    {
      attrs.mode &= (~global_fs_info.umask);
    }

  /* >> Then, convert the attribute set to your FS format << */
  int flags = 0;
  struct stat stats = { 0 };
  if(FSAL_TEST_MASK(attrs.asked_attributes, FSAL_ATTR_MODE))
  {
    flags |= LZFSW_ATTR_MODE;
    stats.st_mode = fsal2unix_mode(attrs.mode);
  }
  if(FSAL_TEST_MASK(attrs.asked_attributes, FSAL_ATTR_OWNER))
  {
    flags |= LZFSW_ATTR_UID;
    stats.st_uid = attrs.owner;
  }
  if(FSAL_TEST_MASK(attrs.asked_attributes, FSAL_ATTR_GROUP))
  {
    flags |= LZFSW_ATTR_GID;
    stats.st_gid = attrs.group;
  }
  if(FSAL_TEST_MASK(attrs.asked_attributes, FSAL_ATTR_ATIME))
  {
    flags |= LZFSW_ATTR_ATIME;
    stats.st_atime = attrs.atime.seconds;
  }
  if(FSAL_TEST_MASK(attrs.asked_attributes, FSAL_ATTR_MTIME))
  {
    flags |= LZFSW_ATTR_MTIME;
    stats.st_mtime = attrs.mtime.seconds;
  }

  TakeTokenFSCall();

  struct stat new_stat = { 0 };
  /**@TODO: use the new_stat info ! */
  rc = libzfswrap_setattr(p_context->export_context->p_vfs, &p_context->user_credential.cred,
                          filehandle->data.zfs_handle, &stats, flags, &new_stat);

  ReleaseTokenFSCall();

  /* >> convert error code, and return on error << */
  if(rc)
    Return(posix2fsal_error(rc), 0, INDEX_FSAL_setattrs);

  /* >> Optionaly fill output attributes.
   * If your filesystem setattr call doesn't
   * return object attributes, you may do something
   * like that : << */

  if(object_attributes)
    {

      status = ZFSFSAL_getattrs(filehandle, p_context, object_attributes);

      /* on error, we set a special bit in the mask. */
      if(FSAL_IS_ERROR(status))
        {
          FSAL_CLEAR_MASK(object_attributes->asked_attributes);
          FSAL_SET_MASK(object_attributes->asked_attributes, FSAL_ATTR_RDATTR_ERR);
        }

    }

  Return(ERR_FSAL_NO_ERROR, 0, INDEX_FSAL_setattrs);

}

/**
 * FSAL_getetxattrs:
 * Get attributes for the object specified by its filehandle.
 *
 * \param filehandle (input):
 *        The handle of the object to get parameters.
 * \param cred (input):
 *        Authentication context for the operation (user,...).
 * \param object_attributes (mandatory input/output):
 *        The retrieved attributes for the object.
 *        As input, it defines the attributes that the caller
 *        wants to retrieve (by positioning flags into this structure)
 *        and the output is built considering this input
 *        (it fills the structure according to the flags it contains).
 *
 * \return Major error codes :
 *        - ERR_FSAL_NO_ERROR     (no error)
 *        - Another error code if an error occured.
 */
fsal_status_t ZFSFSAL_getextattrs(zfsfsal_handle_t * p_filehandle, /* IN */
                                  zfsfsal_op_context_t * p_context,        /* IN */
                                  fsal_extattrib_list_t * p_object_attributes /* OUT */
    )
{
  if( p_object_attributes->asked_attributes & FSAL_ATTR_GENERATION )
    p_object_attributes->generation = p_filehandle->data.zfs_handle.generation;

  Return(ERR_FSAL_NO_ERROR, 0, INDEX_FSAL_getextattrs);
} /* ZFSFSAL_getextattrs */
