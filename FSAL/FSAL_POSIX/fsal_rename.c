/*
 * vim:expandtab:shiftwidth=8:tabstop=8:
 */

/**
 *
 * \file    fsal_rename.c
 * \author  $Author: leibovic $
 * \date    $Date: 2006/01/24 13:45:37 $
 * \version $Revision: 1.9 $
 * \brief   object renaming/moving function.
 *
 */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "fsal.h"
#include "fsal_internal.h"
#include "fsal_convert.h"

/**
 * FSAL_rename:
 * Change name and/or parent dir of a filesystem object.
 *
 * \param old_parentdir_handle (input):
 *        Source parent directory of the object is to be moved/renamed.
 * \param p_old_name (input):
 *        Pointer to the current name of the object to be moved/renamed.
 * \param new_parentdir_handle (input):
 *        Target parent directory for the object.
 * \param p_new_name (input):
 *        Pointer to the new name for the object.
 * \param cred (input):
 *        Authentication context for the operation (user,...).
 * \param src_dir_attributes (optionnal input/output): 
 *        Post operation attributes for the source directory.
 *        As input, it defines the attributes that the caller
 *        wants to retrieve (by positioning flags into this structure)
 *        and the output is built considering this input
 *        (it fills the structure according to the flags it contains).
 *        May be NULL.
 * \param tgt_dir_attributes (optionnal input/output): 
 *        Post operation attributes for the target directory.
 *        As input, it defines the attributes that the caller
 *        wants to retrieve (by positioning flags into this structure)
 *        and the output is built considering this input
 *        (it fills the structure according to the flags it contains).
 *        May be NULL.
 *
 * \return Major error codes :
 *        - ERR_FSAL_NO_ERROR     (no error)
 *        - Another error code if an error occured.
 */

fsal_status_t POSIXFSAL_rename(posixfsal_handle_t * p_old_parentdir_handle,     /* IN */
                               fsal_name_t * p_old_name,        /* IN */
                               posixfsal_handle_t * p_new_parentdir_handle,     /* IN */
                               fsal_name_t * p_new_name,        /* IN */
                               posixfsal_op_context_t * p_context,      /* IN */
                               fsal_attrib_list_t * p_src_dir_attributes,       /* [ IN/OUT ] */
                               fsal_attrib_list_t * p_tgt_dir_attributes        /* [ IN/OUT ] */
    )
{

  int rc, errsv;
  fsal_status_t status;
  fsal_posixdb_status_t statusdb;
  struct stat old_parent_buffstat, new_parent_buffstat, buffstat;
  fsal_path_t old_fsalpath, new_fsalpath;
  fsal_posixdb_fileinfo_t info;

  /* sanity checks.
   * note : src/tgt_dir_attributes are optional.
   */
  if(!p_old_parentdir_handle ||
     !p_new_parentdir_handle || !p_old_name || !p_new_name || !p_context)
    Return(ERR_FSAL_FAULT, 0, INDEX_FSAL_rename);

  /**************************************
   * Build the old path and the new one *
   **************************************/
  status =
      fsal_internal_getPathFromHandle(p_context, p_old_parentdir_handle, 1, &old_fsalpath,
                                      &old_parent_buffstat);
  if(FSAL_IS_ERROR(status))
    Return(status.major, status.minor, INDEX_FSAL_rename);

  /* optimisation : don't do the work two times if source dir = dest dir  */
  if(!POSIXFSAL_handlecmp(p_old_parentdir_handle, p_new_parentdir_handle, &status))
    {
      FSAL_pathcpy(&new_fsalpath, &old_fsalpath);
      new_parent_buffstat = old_parent_buffstat;
    }
  else
    {
      status =
          fsal_internal_getPathFromHandle(p_context, p_new_parentdir_handle, 1,
                                          &new_fsalpath, &new_parent_buffstat);
      if(FSAL_IS_ERROR(status))
        Return(status.major, status.minor, INDEX_FSAL_rename);
    }

  status = fsal_internal_appendFSALNameToFSALPath(&old_fsalpath, p_old_name);
  if(FSAL_IS_ERROR(status))
    Return(status.major, status.minor, INDEX_FSAL_mkdir);
  status = fsal_internal_appendFSALNameToFSALPath(&new_fsalpath, p_new_name);
  if(FSAL_IS_ERROR(status))
    Return(status.major, status.minor, INDEX_FSAL_mkdir);

  TakeTokenFSCall();
  rc = lstat(old_fsalpath.path, &buffstat);
  errsv = errno;
  ReleaseTokenFSCall();
  if(rc)
    Return(posix2fsal_error(errsv), errsv, INDEX_FSAL_rename);

  if(FSAL_IS_ERROR(status = fsal_internal_posix2posixdb_fileinfo(&buffstat, &info)))
    Return(status.major, status.minor, INDEX_FSAL_rename);

  /********************
   * Check credential *
   ********************/

  if(FSAL_IS_ERROR
     (status =
      fsal_internal_testAccess(p_context, FSAL_W_OK | FSAL_X_OK, &old_parent_buffstat,
                               NULL)))
    Return(status.major, status.minor, INDEX_FSAL_rename);

  if(FSAL_IS_ERROR
     (status =
      fsal_internal_testAccess(p_context, FSAL_W_OK | FSAL_X_OK, &new_parent_buffstat,
                               NULL)))
    Return(status.major, status.minor, INDEX_FSAL_rename);

  /* Check sticky bit on directories */

  if((old_parent_buffstat.st_mode & S_ISVTX)    /* Sticky bit on the directory => the user who wants to delete the file must own it or its parent dir */
     && old_parent_buffstat.st_uid != p_context->credential.user
     && buffstat.st_uid != p_context->credential.user && p_context->credential.user != 0)
    Return(ERR_FSAL_ACCESS, 0, INDEX_FSAL_rename);

  if(new_parent_buffstat.st_mode & S_ISVTX)
    {                           /* Sticky bit on the directory => the user who wants to delete the file must own it or its parent dir */
      TakeTokenFSCall();
      rc = lstat(new_fsalpath.path, &buffstat);
      errsv = errno;
      ReleaseTokenFSCall();
      if(rc)
        {
          if(errsv != ENOENT)
            Return(posix2fsal_error(errsv), errsv, INDEX_FSAL_rename);
        }
      else if(new_parent_buffstat.st_uid != p_context->credential.user
              && buffstat.st_uid != p_context->credential.user
              && p_context->credential.user != 0)
        Return(ERR_FSAL_ACCESS, 0, INDEX_FSAL_rename);
    }
  /*************************************
   * Rename the file on the filesystem *
   *************************************/
  TakeTokenFSCall();
  rc = rename(old_fsalpath.path, new_fsalpath.path);
  errsv = errno;
  ReleaseTokenFSCall();

  if(rc)
    Return(posix2fsal_error(errsv), errsv, INDEX_FSAL_rename);

  /***********************************
   * Rename the file in the database *
   ***********************************/
  statusdb = fsal_posixdb_replace(p_context->p_conn,
                                  &info,
                                  p_old_parentdir_handle,
                                  p_old_name, p_new_parentdir_handle, p_new_name);

  switch (statusdb.major)
    {
    case ERR_FSAL_POSIXDB_NOENT:
    case ERR_FSAL_POSIXDB_NOERR:
      break;
    default:
      if(FSAL_IS_ERROR(status = posixdb2fsal_error(statusdb)))
        Return(status.major, status.minor, INDEX_FSAL_rename);
    }

  /***********************
   * Fill the attributes *
   ***********************/

  if(p_src_dir_attributes)
    {

      status =
          POSIXFSAL_getattrs(p_old_parentdir_handle, p_context, p_src_dir_attributes);

      if(FSAL_IS_ERROR(status))
        {
          FSAL_CLEAR_MASK(p_src_dir_attributes->asked_attributes);
          FSAL_SET_MASK(p_src_dir_attributes->asked_attributes, FSAL_ATTR_RDATTR_ERR);
        }

    }

  if(p_tgt_dir_attributes)
    {

      status =
          POSIXFSAL_getattrs(p_new_parentdir_handle, p_context, p_tgt_dir_attributes);

      if(FSAL_IS_ERROR(status))
        {
          FSAL_CLEAR_MASK(p_tgt_dir_attributes->asked_attributes);
          FSAL_SET_MASK(p_tgt_dir_attributes->asked_attributes, FSAL_ATTR_RDATTR_ERR);
        }

    }

  /* OK */
  Return(ERR_FSAL_NO_ERROR, 0, INDEX_FSAL_rename);

}
