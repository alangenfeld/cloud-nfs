/*
 *
 *
 * Copyright CEA/DAM/DIF  (2010)
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
 * \file    fsal_glue.h
 * \date    $Date: 2010/07/01 12:45:27 $
 *
 *
 */

#ifndef _FSAL_GLUE_H
#define _FSAL_GLUE_H

#include "fsal_glue_const.h"

/* In the "static" case, original types are used, this is safer */
#ifdef _USE_SHARED_FSAL

typedef struct
{
  char data[FSAL_HANDLE_T_SIZE];
} fsal_handle_t;

typedef fsal_handle_t fsal_handle_storage_t ;

typedef struct
{
  void *export_context;
  char data[FSAL_OP_CONTEXT_T_SIZE];
} fsal_op_context_t;

typedef struct
{
  char data[FSAL_DIR_T_SIZE];
} fsal_dir_t;

typedef struct
{
  char data[FSAL_EXPORT_CONTEXT_T_SIZE];
} fsal_export_context_t;

typedef struct
{
  char data[FSAL_FILE_T_SIZE];
} fsal_file_t;

typedef struct
{
  char data[FSAL_COOKIE_T_SIZE];
} fsal_cookie_t;

typedef struct
{
  char data[FSAL_LOCKDESC_T_SIZE];
} fsal_lockdesc_t;

typedef struct
{
  char data[FSAL_CRED_T_SIZE];
} fsal_cred_t;

typedef struct
{
  char data[FSAL_FS_SPECIFIC_INITINFO_T];
} fs_specific_initinfo_t;

#endif                          /* USE_SHARED_FSAL */

#endif                          /* _FSAL_GLUE_H */
