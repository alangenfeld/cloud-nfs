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
 * ------------- 
 */

/**
 *
 * \file    fsal_init.c
 * \author  $Author: leibovic $
 * \date    $Date: 2006/01/24 13:45:37 $
 * \version $Revision: 1.20 $
 * \brief   Initialization functions.
 *
 */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "fsal.h"
#include "fsal_internal.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

/* Macros for analysing parameters. */
#define SET_BITMAP_PARAM( api_cfg, p_init_info, _field )      \
    switch( (p_init_info)->behaviors._field ){                \
      case FSAL_INIT_FORCE_VALUE :                            \
        /* force the value in any case */                     \
        api_cfg._field = (p_init_info)->hpss_config._field;   \
        break;                                                \
      case FSAL_INIT_MAX_LIMIT :                              \
        /* remove the flags not specified by user (AND) */    \
        api_cfg._field &= (p_init_info)->hpss_config._field;  \
        break;                                                \
      case FSAL_INIT_MIN_LIMIT :                              \
        /* add the flags specified by user (OR) */            \
        api_cfg._field |= (p_init_info)->hpss_config._field;  \
        break;                                                \
    /* In the other cases, we keep the default value. */      \
    }                                                         \


#define SET_INTEGER_PARAM( api_cfg, p_init_info, _field )         \
    switch( (p_init_info)->behaviors._field ){                    \
    case FSAL_INIT_FORCE_VALUE :                                  \
        /* force the value in any case */                         \
        api_cfg._field = (p_init_info)->hpss_config._field;       \
        break;                                                \
    case FSAL_INIT_MAX_LIMIT :                                    \
      /* check the higher limit */                                \
      if ( api_cfg._field > (p_init_info)->hpss_config._field )   \
        api_cfg._field = (p_init_info)->hpss_config._field ;      \
        break;                                                \
    case FSAL_INIT_MIN_LIMIT :                                    \
      /* check the lower limit */                                 \
      if ( api_cfg._field < (p_init_info)->hpss_config._field )   \
        api_cfg._field = (p_init_info)->hpss_config._field ;      \
        break;                                                \
    /* In the other cases, we keep the default value. */          \
    }                                                             \


#define SET_STRING_PARAM( api_cfg, p_init_info, _field )          \
    switch( (p_init_info)->behaviors._field ){                    \
    case FSAL_INIT_FORCE_VALUE :                                  \
      /* force the value in any case */                           \
      strcpy(api_cfg._field,(p_init_info)->hpss_config._field);   \
      break;                                                \
    /* In the other cases, we keep the default value. */          \
    }                                                             \


/**
 * FSAL_Init : Initializes the FileSystem Abstraction Layer.
 *
 * \param init_info (input, fsal_parameter_t *) :
 *        Pointer to a structure that contains
 *        all initialization parameters for the FSAL.
 *        Specifically, it contains settings about
 *        the filesystem on which the FSAL is based,
 *        security settings, logging policy and outputs,
 *        and other general FSAL options.
 *
 * \return Major error codes :
 *         ERR_FSAL_NO_ERROR     (initialisation OK)
 *         ERR_FSAL_FAULT        (init_info pointer is null)
 *         ERR_FSAL_SERVERFAULT  (misc FSAL error)
 *         ERR_FSAL_ALREADY_INIT (The FS is already initialized)
 *         ERR_FSAL_BAD_INIT     (FS specific init error,
 *                                minor error code gives the reason
 *                                for this error.)
 *         ERR_FSAL_SEC_INIT     (Security context init error).
 */
fsal_status_t GPFSFSAL_Init(fsal_parameter_t * init_info    /* IN */
    )
{
  char *fshandle;
  size_t fshandlelen = 0;

  fsal_status_t status;
  int rc = 0;

  /* sanity check.  */
  if(!init_info)
    Return(ERR_FSAL_FAULT, 0, INDEX_FSAL_Init);

  /* save open-by-handle char device */
  memcpy(open_by_handle_path, init_info->fs_specific_info.open_by_handle_dev_file,
         MAXPATHLEN);
  open_by_handle_fd = open(init_info->fs_specific_info.open_by_handle_dev_file, O_RDONLY);
  if(open_by_handle_fd < 0)
    {
      LogMajor(COMPONENT_FSAL,
               "FSAL INIT: ERROR: Could not open open-by-handle character device file at %s: rc = %d",
               init_info->fs_specific_info.open_by_handle_dev_file, errno);
      ReturnCode(ERR_FSAL_INVAL, 0);
    }

  /* proceeds FSAL internal initialization */
  status = fsal_internal_init_global(&(init_info->fsal_info),
                                     &(init_info->fs_common_info),
                                     &(init_info->fs_specific_info));
  if(FSAL_IS_ERROR(status))
    Return(status.major, status.minor, INDEX_FSAL_Init);

  /* Regular exit */
  Return(ERR_FSAL_NO_ERROR, 0, INDEX_FSAL_Init);

}

/* To be called before exiting */
fsal_status_t GPFSFSAL_terminate()
{
  ReturnCode(ERR_FSAL_NO_ERROR, 0);
}
