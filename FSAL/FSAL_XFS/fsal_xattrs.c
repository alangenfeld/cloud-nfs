/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil; -*-
 * vim:expandtab:shiftwidth=4:tabstop=4:
 */
/**
 * \file    fsal_xattrs.c
 * \author  $Author: leibovic $
 * \date    $Date: 2007/08/23 $
 * \version $Revision: 1.0 $
 * \brief   Extended attributes functions.
 */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "fsal.h"
#include "fsal_internal.h"
#include "fsal_convert.h"
#include "stuff_alloc.h"

#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <attr/xattr.h>

/* generic definitions for extended attributes */

#define XATTR_FOR_FILE     0x00000001
#define XATTR_FOR_DIR      0x00000002
#define XATTR_FOR_SYMLINK  0x00000004
#define XATTR_FOR_ALL      0x0000000F
#define XATTR_RO           0x00000100
#define XATTR_RW           0x00000200

/* function for getting an attribute value */

typedef int (*xattr_getfunc_t) (xfsfsal_handle_t *,     /* object handle */
                                xfsfsal_op_context_t *, /* context */
                                caddr_t,        /* output buff */
                                size_t, /* output buff size */
                                size_t *,       /* output size */
                                void *arg);     /* optionnal argument */

typedef int (*xattr_setfunc_t) (xfsfsal_handle_t *,     /* object handle */
                                xfsfsal_op_context_t *, /* context */
                                caddr_t,        /* input buff */
                                size_t, /* input size */
                                int,    /* creation flag */
                                void *arg);     /* optionnal argument */

typedef struct fsal_xattr_def__
{
  char xattr_name[FSAL_MAX_NAME_LEN];
  xattr_getfunc_t get_func;
  xattr_setfunc_t set_func;
  int flags;
  void *arg;
} fsal_xattr_def_t;

/*
 * DEFINE GET/SET FUNCTIONS
 */

int print_xfshandle(xfsfsal_handle_t * p_objecthandle,  /* object handle */
                    xfsfsal_op_context_t * p_context,   /* IN */
                    caddr_t buffer_addr,        /* IN/OUT */
                    size_t buffer_size, /* IN */
                    size_t * p_output_size,     /* OUT */
                    void *arg)
{
  *p_output_size =
      snprintmem(buffer_addr, buffer_size, p_objecthandle->data.handle_val,
                 p_objecthandle->data.handle_len);
  strncat((char *)buffer_addr, "\n", buffer_size);
  (*p_output_size) += 1;

  return 0;
}                               /* print_fid */

/* DEFINE HERE YOUR ATTRIBUTES LIST */

static fsal_xattr_def_t xattr_list[] = {
  {"xfshandle", print_xfshandle, NULL, XATTR_FOR_ALL | XATTR_RO, NULL},
};

#define XATTR_COUNT 1

/* we assume that this number is < 254 */
#if ( XATTR_COUNT > 254 )
#error "ERROR: xattr count > 254"
#endif

/* YOUR SHOULD NOT HAVE TO MODIFY THE FOLLOWING FUNCTIONS */

/* test if an object has a given attribute */
static int do_match_type(int xattr_flag, fsal_nodetype_t obj_type)
{
  switch (obj_type)
    {
    case FSAL_TYPE_FILE:
      return ((xattr_flag & XATTR_FOR_FILE) == XATTR_FOR_FILE);

    case FSAL_TYPE_DIR:
      return ((xattr_flag & XATTR_FOR_DIR) == XATTR_FOR_DIR);

    case FSAL_TYPE_LNK:
      return ((xattr_flag & XATTR_FOR_SYMLINK) == XATTR_FOR_SYMLINK);

    default:
      return ((xattr_flag & XATTR_FOR_ALL) == XATTR_FOR_ALL);
    }
}

static int attr_is_read_only(unsigned int attr_index)
{
  if(attr_index < XATTR_COUNT)
    {
      if(xattr_list[attr_index].flags & XATTR_RO)
        return TRUE;
    }
  /* else : standard xattr */
  return FALSE;
}

static int file_attributes_to_xattr_attrs(fsal_attrib_list_t * file_attrs,
                                          fsal_attrib_list_t * p_xattr_attrs,
                                          unsigned int attr_index)
{
  /* supported attributes are:
   * - owner (same as the objet)
   * - group (same as the objet)
   * - type FSAL_TYPE_XATTR
   * - fileid (attr index ? or (fileid^((index+1)<<24)) )
   * - mode (config & file)
   * - atime, mtime, ctime = these of the object ?
   * - size=1block, used=1block
   * - rdev=0
   * - nlink=1
   */
  fsal_attrib_mask_t supported = FSAL_ATTR_SUPPATTR | FSAL_ATTR_MODE | FSAL_ATTR_FILEID
      | FSAL_ATTR_TYPE | FSAL_ATTR_OWNER | FSAL_ATTR_GROUP
      | FSAL_ATTR_ATIME | FSAL_ATTR_MTIME | FSAL_ATTR_CTIME
      | FSAL_ATTR_CREATION | FSAL_ATTR_CHGTIME | FSAL_ATTR_SIZE
      | FSAL_ATTR_SPACEUSED | FSAL_ATTR_NUMLINKS | FSAL_ATTR_RAWDEV | FSAL_ATTR_FSID;
  fsal_attrib_mask_t unsupp;

  /* only those supported by filesystem */
  supported &= global_fs_info.supported_attrs;

  if(p_xattr_attrs->asked_attributes == 0)
    {
      p_xattr_attrs->asked_attributes = supported;

      LogCrit(COMPONENT_FSAL,
                        "Error: p_xattr_attrs->asked_attributes was 0 in %s() line %d, file %s",
                        __FUNCTION__, __LINE__, __FILE__);
    }

  unsupp = p_xattr_attrs->asked_attributes & (~supported);

  if(unsupp)
    {
      LogDebug(COMPONENT_FSAL,
                        "Asking for unsupported attributes in %s(): %#llX removing it from asked attributes",
                        __FUNCTION__, unsupp);

      p_xattr_attrs->asked_attributes &= (~unsupp);
    }

  if(p_xattr_attrs->asked_attributes & FSAL_ATTR_SUPPATTR)
    p_xattr_attrs->supported_attributes = supported;

  if(p_xattr_attrs->asked_attributes & FSAL_ATTR_MODE)
    {
      p_xattr_attrs->mode = file_attrs->mode & global_fs_info.xattr_access_rights;

      if(attr_is_read_only(attr_index))
        p_xattr_attrs->mode &= ~(0222);
    }

  if(p_xattr_attrs->asked_attributes & FSAL_ATTR_FILEID)
    {
      unsigned int i;
      unsigned long hash = attr_index + 1;
      char *str = (char *)&file_attrs->fileid;

      for(i = 0; i < sizeof(p_xattr_attrs->fileid); i++, str++)
        {
          hash = (hash << 5) - hash + (unsigned long)(*str);
        }
      p_xattr_attrs->fileid = hash;
    }

  if(p_xattr_attrs->asked_attributes & FSAL_ATTR_TYPE)
    p_xattr_attrs->type = FSAL_TYPE_XATTR;

  if(p_xattr_attrs->asked_attributes & FSAL_ATTR_OWNER)
    p_xattr_attrs->owner = file_attrs->owner;

  if(p_xattr_attrs->asked_attributes & FSAL_ATTR_GROUP)
    p_xattr_attrs->group = file_attrs->group;

  if(p_xattr_attrs->asked_attributes & FSAL_ATTR_ATIME)
    p_xattr_attrs->atime = file_attrs->atime;

  if(p_xattr_attrs->asked_attributes & FSAL_ATTR_MTIME)
    p_xattr_attrs->mtime = file_attrs->mtime;

  if(p_xattr_attrs->asked_attributes & FSAL_ATTR_CTIME)
    p_xattr_attrs->ctime = file_attrs->ctime;

  if(p_xattr_attrs->asked_attributes & FSAL_ATTR_CREATION)
    p_xattr_attrs->creation = file_attrs->creation;

  if(p_xattr_attrs->asked_attributes & FSAL_ATTR_CHGTIME)
    p_xattr_attrs->chgtime = file_attrs->chgtime;

  if(p_xattr_attrs->asked_attributes & FSAL_ATTR_SIZE)
    p_xattr_attrs->filesize = DEV_BSIZE;

  if(p_xattr_attrs->asked_attributes & FSAL_ATTR_SPACEUSED)
    p_xattr_attrs->spaceused = DEV_BSIZE;

  if(p_xattr_attrs->asked_attributes & FSAL_ATTR_NUMLINKS)
    p_xattr_attrs->numlinks = 1;

  if(p_xattr_attrs->asked_attributes & FSAL_ATTR_RAWDEV)
    {
      p_xattr_attrs->rawdev.major = 0;
      p_xattr_attrs->rawdev.minor = 0;
    }

  if(p_xattr_attrs->asked_attributes & FSAL_ATTR_FSID)
    {
      p_xattr_attrs->fsid = file_attrs->fsid;
    }

  /* if mode==0, then owner is set to root and mode is set to 0600 */
  if((p_xattr_attrs->asked_attributes & FSAL_ATTR_OWNER)
     && (p_xattr_attrs->asked_attributes & FSAL_ATTR_MODE) && (p_xattr_attrs->mode == 0))
    {
      p_xattr_attrs->owner = 0;
      p_xattr_attrs->mode = 0600;
      if(attr_is_read_only(attr_index))
        p_xattr_attrs->mode &= ~(0200);
    }

  return 0;

}

/**
 * Get the attributes of an extended attribute from its index.
 *
 * \param p_objecthandle Handle of the object you want to get attribute for.
 * \param p_context pointer to the current security context.
 * \param xattr_cookie xattr's cookie (as returned by listxattrs).
 * \param p_attrs xattr's attributes.
 */
fsal_status_t XFSFSAL_GetXAttrAttrs(xfsfsal_handle_t * p_objecthandle,  /* IN */
                                    xfsfsal_op_context_t * p_context,   /* IN */
                                    unsigned int xattr_id,      /* IN */
                                    fsal_attrib_list_t * p_attrs
                                          /**< IN/OUT xattr attributes (if supported) */
    )
{
  int rc;
  fsal_status_t st;
  fsal_attrib_list_t file_attrs;

  /* sanity checks */
  if(!p_objecthandle || !p_context || !p_attrs)
    Return(ERR_FSAL_FAULT, 0, INDEX_FSAL_GetXAttrAttrs);

  /* object attributes we want to retrieve from parent */
  file_attrs.asked_attributes = FSAL_ATTR_MODE | FSAL_ATTR_FILEID | FSAL_ATTR_OWNER
      | FSAL_ATTR_GROUP | FSAL_ATTR_ATIME | FSAL_ATTR_MTIME | FSAL_ATTR_TYPE
      | FSAL_ATTR_CTIME | FSAL_ATTR_CREATION | FSAL_ATTR_CHGTIME | FSAL_ATTR_FSID;

  /* don't retrieve attributes not asked */
  file_attrs.asked_attributes &= p_attrs->asked_attributes;

  st = XFSFSAL_getattrs(p_objecthandle, p_context, &file_attrs);

  if(FSAL_IS_ERROR(st))
    Return(st.major, st.minor, INDEX_FSAL_GetXAttrAttrs);

  /* check that this index match the type of entry */
  if(xattr_id < XATTR_COUNT
     && !do_match_type(xattr_list[xattr_id].flags, file_attrs.type))
    {
      Return(ERR_FSAL_INVAL, 0, INDEX_FSAL_GetXAttrAttrs);
    }
  else if(xattr_id >= XATTR_COUNT)
    {
      /* This is user defined xattr */
      LogFullDebug(COMPONENT_FSAL,
                        "Getting attributes for xattr #%u", xattr_id - XATTR_COUNT);
    }

  if((rc = file_attributes_to_xattr_attrs(&file_attrs, p_attrs, xattr_id)))
    {
      Return(ERR_FSAL_INVAL, rc, INDEX_FSAL_GetXAttrAttrs);
    }

  Return(ERR_FSAL_NO_ERROR, 0, INDEX_FSAL_GetXAttrAttrs);

}                               /* FSAL_GetXAttrAttrs */

/**
 * Retrieves the list of extended attributes for an object in the filesystem.
 * 
 * \param p_objecthandle Handle of the object we want to get extended attributes.
 * \param cookie index of the next entry to be returned.
 * \param p_context pointer to the current security context.
 * \param xattrs_tab a table for storing extended attributes list to.
 * \param xattrs_tabsize the maximum number of xattr entries that xattrs_tab
 *            can contain.
 * \param p_nb_returned the number of xattr entries actually stored in xattrs_tab.
 * \param end_of_list this boolean indicates that the end of xattrs list has been reached.
 */
fsal_status_t XFSFSAL_ListXAttrs(xfsfsal_handle_t * p_objecthandle,     /* IN */
                                 unsigned int cookie,   /* IN */
                                 xfsfsal_op_context_t * p_context,      /* IN */
                                 fsal_xattrent_t * xattrs_tab,  /* IN/OUT */
                                 unsigned int xattrs_tabsize,   /* IN */
                                 unsigned int *p_nb_returned,   /* OUT */
                                 int *end_of_list       /* OUT */
    )
{
  unsigned int index;
  unsigned int out_index;
  fsal_status_t st;
  fsal_attrib_list_t file_attrs;
  int fd;

  char names[MAXPATHLEN], *ptr;
  size_t namesize;
  int xattr_idx;

  /* sanity checks */
  if(!p_objecthandle || !p_context || !xattrs_tab || !p_nb_returned || !end_of_list)
    Return(ERR_FSAL_FAULT, 0, INDEX_FSAL_ListXAttrs);

  /* object attributes we want to retrieve from parent */
  file_attrs.asked_attributes = FSAL_ATTR_MODE | FSAL_ATTR_FILEID | FSAL_ATTR_OWNER
      | FSAL_ATTR_GROUP | FSAL_ATTR_ATIME | FSAL_ATTR_MTIME | FSAL_ATTR_TYPE
      | FSAL_ATTR_CTIME | FSAL_ATTR_CREATION | FSAL_ATTR_CHGTIME | FSAL_ATTR_FSID;

  /* don't retrieve unsuipported attributes */
  file_attrs.asked_attributes &= global_fs_info.supported_attrs;

  st = XFSFSAL_getattrs(p_objecthandle, p_context, &file_attrs);

  if(FSAL_IS_ERROR(st))
    Return(st.major, st.minor, INDEX_FSAL_ListXAttrs);

  for(index = cookie, out_index = 0;
      index < XATTR_COUNT && out_index < xattrs_tabsize; index++)
    {
      if(do_match_type(xattr_list[index].flags, file_attrs.type))
        {
          /* fills an xattr entry */
          xattrs_tab[out_index].xattr_id = index;
          FSAL_str2name(xattr_list[index].xattr_name, FSAL_MAX_NAME_LEN,
                        &xattrs_tab[out_index].xattr_name);
          xattrs_tab[out_index].xattr_cookie = index + 1;

          /* set asked attributes (all supported) */
          xattrs_tab[out_index].attributes.asked_attributes =
              global_fs_info.supported_attrs;

          if(file_attributes_to_xattr_attrs
             (&file_attrs, &xattrs_tab[out_index].attributes, index))
            {
              /* set error flag */
              xattrs_tab[out_index].attributes.asked_attributes = FSAL_ATTR_RDATTR_ERR;
            }

          /* next output slot */
          out_index++;
        }
    }

  /* save a call if output array is full */
  if(out_index == xattrs_tabsize)
    {
      *end_of_list = FALSE;
      *p_nb_returned = out_index;
      Return(ERR_FSAL_NO_ERROR, 0, INDEX_FSAL_ListXAttrs);
    }

  /* get the path of the file in Lustre */
  TakeTokenFSCall();
  st = fsal_internal_handle2fd(p_context, p_objecthandle, &fd, O_RDWR);
  ReleaseTokenFSCall();
  if(FSAL_IS_ERROR(st))
    ReturnStatus(st, INDEX_FSAL_ListXAttrs);

  /* get xattrs */

  TakeTokenFSCall();
  namesize = flistxattr(fd, names, sizeof(names));
  ReleaseTokenFSCall();

  if(namesize >= 0)
    {
      size_t len = 0;

      errno = 0;

      for(ptr = names, xattr_idx = 0;
          (ptr < names + namesize) && (out_index < xattrs_tabsize);
          xattr_idx++, ptr += len + 1)
        {
          len = strlen(ptr);
          index = XATTR_COUNT + xattr_idx;

          /* skip if index is before cookie */
          if(index < cookie)
            continue;

          /* fills an xattr entry */
          xattrs_tab[out_index].xattr_id = index;
          FSAL_str2name(ptr, len + 1, &xattrs_tab[out_index].xattr_name);
          xattrs_tab[out_index].xattr_cookie = index + 1;

          /* set asked attributes (all supported) */
          xattrs_tab[out_index].attributes.asked_attributes =
              global_fs_info.supported_attrs;

          if(file_attributes_to_xattr_attrs(&file_attrs,
                                            &xattrs_tab[out_index].attributes, index))
            {
              /* set error flag */
              xattrs_tab[out_index].attributes.asked_attributes = FSAL_ATTR_RDATTR_ERR;
            }

          /* next output slot */
          out_index++;
        }
      /* all xattrs are in the output array */
      if(ptr >= names + namesize)
        *end_of_list = TRUE;
      else
        *end_of_list = FALSE;
    }
  else                          /* no xattrs */
    *end_of_list = TRUE;

  *p_nb_returned = out_index;

  close(fd);
  Return(ERR_FSAL_NO_ERROR, 0, INDEX_FSAL_ListXAttrs);

}

static int xattr_id_to_name(int fd, unsigned int xattr_id, char *name)
{
  unsigned int index;
  unsigned int curr_idx;
  char names[MAXPATHLEN], *ptr;
  size_t namesize;
  size_t len = 0;

  if(xattr_id < XATTR_COUNT)
    return ERR_FSAL_INVAL;

  index = xattr_id - XATTR_COUNT;

  /* get xattrs */

  TakeTokenFSCall();
  namesize = flistxattr(fd, names, sizeof(names));
  ReleaseTokenFSCall();

  if(namesize < 0)
    return ERR_FSAL_NOENT;

  errno = 0;

  for(ptr = names, curr_idx = 0; ptr < names + namesize; curr_idx++, ptr += len + 1)
    {
      len = strlen(ptr);
      if(curr_idx == index)
        {
          strcpy(name, ptr);
          return ERR_FSAL_NO_ERROR;
        }
    }
  return ERR_FSAL_NOENT;
}

/**
 *  return index if found,
 *  negative value on error.
 */
static int xattr_name_to_id(int fd, const char *name)
{
  unsigned int i;
  char names[MAXPATHLEN], *ptr;
  size_t namesize;
  size_t len = 0;

  /* get xattrs */

  TakeTokenFSCall();
  namesize = flistxattr(fd, names, sizeof(names));
  ReleaseTokenFSCall();

  if(namesize < 0)
    return -ERR_FSAL_NOENT;

  for(ptr = names, i = 0; ptr < names + namesize; i++, ptr += strlen(ptr) + 1)
    {
      if(!strcmp(name, ptr))
        return i + XATTR_COUNT;
    }
  return -ERR_FSAL_NOENT;
}

static int xattr_format_value(caddr_t buffer, size_t * datalen, size_t maxlen)
{
  size_t size_in = *datalen;
  size_t len = strnlen((char *)buffer, size_in);
  int i;

  if(len == size_in - 1 || len == size_in)
    {
      int ascii = TRUE;
      char *str = buffer;
      int i;

      for(i = 0; i < len; i++)
        {
          if(!isprint(str[i]) && !isspace(str[i]))
            {
              ascii = FALSE;
              break;
            }
        }

      if(ascii)
        {
          *datalen = size_in;
          /* add additional '\n', if missing */
          if((size_in + 1 < maxlen) && (str[len - 1] != '\n'))
            {
              str[len] = '\n';
              str[len + 1] = '\0';
              (*datalen) += 2;
            }
          return ERR_FSAL_NO_ERROR;
        }
    }

  /* byte, word, 32 or 64 bits */
  if(size_in == 1)
    {
      unsigned char val = *((unsigned char *)buffer);
      *datalen = 1 + snprintf((char *)buffer, maxlen, "%hhu\n", val);
      return ERR_FSAL_NO_ERROR;
    }
  else if(size_in == 2)
    {
      unsigned short val = *((unsigned short *)buffer);
      *datalen = 1 + snprintf((char *)buffer, maxlen, "%hu\n", val);
      return ERR_FSAL_NO_ERROR;
    }
  else if(size_in == 4)
    {
      unsigned int val = *((unsigned int *)buffer);
      *datalen = 1 + snprintf((char *)buffer, maxlen, "%u\n", val);
      return ERR_FSAL_NO_ERROR;
    }
  else if(size_in == 8)
    {
      unsigned long long val = *((unsigned long long *)buffer);
      *datalen = 1 + snprintf((char *)buffer, maxlen, "%llu\n", val);
      return ERR_FSAL_NO_ERROR;
    }
  else
    {
      /* 2 bytes per initial byte +'0x' +\n +\0 */
      char *curr_out;
      char *tmp_buf = (char *)Mem_Alloc(3 * size_in + 4);
      if(!tmp_buf)
        return ERR_FSAL_NOMEM;
      curr_out = tmp_buf;
      curr_out += sprintf(curr_out, "0x");
      /* hexa representation */
      for(i = 0; i < size_in; i++)
        {
          unsigned char *p8 = (unsigned char *)(buffer + i);
          if((i % 4 == 3) && (i != size_in - 1))
            curr_out += sprintf(curr_out, "%02hhX.", *p8);
          else
            curr_out += sprintf(curr_out, "%02hhX", *p8);
        }
      *curr_out = '\n';
      curr_out++;
      *curr_out = '\0';
      curr_out++;
      strncpy((char *)buffer, tmp_buf, maxlen);
      *datalen = strlen(tmp_buf) + 1;
      if(*datalen > maxlen)
        *datalen = maxlen;
      Mem_Free(tmp_buf);
      return ERR_FSAL_NO_ERROR;
    }
}

/**
 * Get the value of an extended attribute from its index.
 *
 * \param p_objecthandle Handle of the object you want to get attribute for.
 * \param xattr_name the name of the attribute to be read.
 * \param p_context pointer to the current security context.
 * \param buffer_addr address of the buffer where the xattr value is to be stored.
 * \param buffer_size size of the buffer where the xattr value is to be stored.
 * \param p_output_size size of the data actually stored into the buffer.
 */
fsal_status_t XFSFSAL_GetXAttrValueById(xfsfsal_handle_t * p_objecthandle,      /* IN */
                                        unsigned int xattr_id,  /* IN */
                                        xfsfsal_op_context_t * p_context,       /* IN */
                                        caddr_t buffer_addr,    /* IN/OUT */
                                        size_t buffer_size,     /* IN */
                                        size_t * p_output_size  /* OUT */
    )
{
  int rc;
  fsal_attrib_list_t file_attrs;
  int fd;
  fsal_status_t st;

  /* sanity checks */
  if(!p_objecthandle || !p_context || !p_output_size || !buffer_addr)
    Return(ERR_FSAL_FAULT, 0, INDEX_FSAL_GetXAttrValue);

  /* get type for checking it */
  file_attrs.asked_attributes = FSAL_ATTR_TYPE;

  st = XFSFSAL_getattrs(p_objecthandle, p_context, &file_attrs);

  if(FSAL_IS_ERROR(st))
    ReturnStatus(st, INDEX_FSAL_GetXAttrValue);

  /* check that this index match the type of entry */
  if((xattr_id < XATTR_COUNT)
     && !do_match_type(xattr_list[xattr_id].flags, file_attrs.type))
    {
      Return(ERR_FSAL_INVAL, 0, INDEX_FSAL_GetXAttrValue);
    }
  else if(xattr_id >= XATTR_COUNT)
    {
      char attr_name[MAXPATHLEN];

      TakeTokenFSCall();
      st = fsal_internal_handle2fd(p_context, p_objecthandle, &fd, O_RDWR);
      ReleaseTokenFSCall();
      if(FSAL_IS_ERROR(st))
        ReturnStatus(st, INDEX_FSAL_GetXAttrValue);

      /* get the name for this attr */
      rc = xattr_id_to_name(fd, xattr_id, attr_name);
      if(rc)
        {
          close(fd);
          Return(rc, errno, INDEX_FSAL_GetXAttrValue);
        }

      rc = fgetxattr(fd, attr_name, buffer_addr, buffer_size);
      if(rc < 0)
        {
          close(fd);
          Return(posix2fsal_error(errno), errno, INDEX_FSAL_GetXAttrValue);
        }

      /* the xattr value can be a binary, or a string.
       * trying to determine its type...
       */
      *p_output_size = rc;
      xattr_format_value(buffer_addr, p_output_size, buffer_size);

      close(fd);
      Return(ERR_FSAL_NO_ERROR, 0, INDEX_FSAL_GetXAttrValue);
    }
  else                          /* built-in attr */
    {
      /* get the value */
      rc = xattr_list[xattr_id].get_func(p_objecthandle, p_context,
                                         buffer_addr, buffer_size,
                                         p_output_size, xattr_list[xattr_id].arg);
      Return(rc, 0, INDEX_FSAL_GetXAttrValue);
    }

}

/**
 *  * Get the index of an xattr based on its name
 *   *
 *   \param p_objecthandle Handle of the object you want to get attribute for.
 *   \param xattr_name the name of the attribute to be read.
 *   \param pxattr_id found xattr_id
 *   
 *   \return ERR_FSAL_NO_ERROR if xattr_name exists, ERR_FSAL_NOENT otherwise
 */

fsal_status_t XFSFSAL_GetXAttrIdByName(xfsfsal_handle_t * p_objecthandle,       /* IN */
                                       const fsal_name_t * xattr_name,  /* IN */
                                       xfsfsal_op_context_t * p_context,        /* IN */
                                       unsigned int *pxattr_id  /* OUT */
    )
{
  fsal_status_t st;
  unsigned int index;
  int rc;
  int found = FALSE;
  int fd;

  /* sanity checks */
  if(!p_objecthandle || !xattr_name)
    Return(ERR_FSAL_FAULT, 0, INDEX_FSAL_GetXAttrValue);

  for(index = 0; index < XATTR_COUNT; index++)
    {
      if(!strcmp(xattr_list[index].xattr_name, xattr_name->name))
        {
          found = TRUE;
          break;
        }
    }

  /* search in xattrs */
  if(!found)
    {

      TakeTokenFSCall();
      st = fsal_internal_handle2fd(p_context, p_objecthandle, &fd, O_RDWR);
      ReleaseTokenFSCall();
      if(FSAL_IS_ERROR(st))
        ReturnStatus(st, INDEX_FSAL_GetXAttrValue);

      errno = 0;
      rc = xattr_name_to_id(fd, xattr_name->name);
      if(rc < 0)
        {
          close(fd);
          Return(-rc, errno, INDEX_FSAL_GetXAttrValue);
        }
      else
        {
          index = rc;
          found = TRUE;
        }
    }

  close(fd);

  if(found)
    {
      *pxattr_id = index;
      Return(ERR_FSAL_NO_ERROR, 0, INDEX_FSAL_GetXAttrValue);
    }
  else
    Return(ERR_FSAL_NOENT, ENOENT, INDEX_FSAL_GetXAttrValue);
}                               /* FSAL_GetXAttrIdByName */

/**
 * Get the value of an extended attribute from its name.
 *
 * \param p_objecthandle Handle of the object you want to get attribute for.
 * \param xattr_name the name of the attribute to be read.
 * \param p_context pointer to the current security context.
 * \param buffer_addr address of the buffer where the xattr value is to be stored.
 * \param buffer_size size of the buffer where the xattr value is to be stored.
 * \param p_output_size size of the data actually stored into the buffer.
 */
fsal_status_t XFSFSAL_GetXAttrValueByName(xfsfsal_handle_t * p_objecthandle,    /* IN */
                                          const fsal_name_t * xattr_name,       /* IN */
                                          xfsfsal_op_context_t * p_context,     /* IN */
                                          caddr_t buffer_addr,  /* IN/OUT */
                                          size_t buffer_size,   /* IN */
                                          size_t * p_output_size        /* OUT */
    )
{
  unsigned int index;
  fsal_attrib_list_t file_attrs;
  fsal_status_t st;
  int rc;
  int fd;

  /* sanity checks */
  if(!p_objecthandle || !p_context || !p_output_size || !buffer_addr || !xattr_name)
    Return(ERR_FSAL_FAULT, 0, INDEX_FSAL_GetXAttrValue);

  /* get type for checking it */
  file_attrs.asked_attributes = FSAL_ATTR_TYPE;

  st = XFSFSAL_getattrs(p_objecthandle, p_context, &file_attrs);

  if(FSAL_IS_ERROR(st))
    ReturnStatus(st, INDEX_FSAL_GetXAttrValue);

  /* look for this name */

  for(index = 0; index < XATTR_COUNT; index++)
    {
      if(do_match_type(xattr_list[index].flags, file_attrs.type)
         && !strcmp(xattr_list[index].xattr_name, xattr_name->name))
        {

          return XFSFSAL_GetXAttrValueById(p_objecthandle, index, p_context, buffer_addr,
                                           buffer_size, p_output_size);
        }
    }

  TakeTokenFSCall();
  st = fsal_internal_handle2fd(p_context, p_objecthandle, &fd, O_RDWR);
  ReleaseTokenFSCall();
  if(FSAL_IS_ERROR(st))
    ReturnStatus(st, INDEX_FSAL_GetXAttrValue);

  /* is it an xattr? */
  rc = fgetxattr(fd, xattr_name->name, buffer_addr, buffer_size);
  if(rc < 0)
    {
      close(fd);
      Return(posix2fsal_error(errno), errno, INDEX_FSAL_GetXAttrValue);
    }
  /* the xattr value can be a binary, or a string.
   * trying to determine its type...
   */
  *p_output_size = rc;
  xattr_format_value(buffer_addr, p_output_size, buffer_size);

  close(fd);
  Return(ERR_FSAL_NO_ERROR, 0, INDEX_FSAL_GetXAttrValue);
}

static void chomp_attr_value(char *str, size_t size)
{
  int len;

  if(str == NULL)
    return;

  /* security: set last char to '\0' */
  str[size - 1] = '\0';

  len = strnlen(str, size);
  if((len > 0) && (str[len - 1] == '\n'))
    str[len - 1] = '\0';
}

fsal_status_t XFSFSAL_SetXAttrValue(xfsfsal_handle_t * p_objecthandle,  /* IN */
                                    const fsal_name_t * xattr_name,     /* IN */
                                    xfsfsal_op_context_t * p_context,   /* IN */
                                    caddr_t buffer_addr,        /* IN */
                                    size_t buffer_size, /* IN */
                                    int create  /* IN */
    )
{
  int rc;
  fsal_status_t st;
  int fd = 0;
  size_t len;

  /* remove final '\n', if any */
  chomp_attr_value((char *)buffer_addr, buffer_size);

  /* build fid path in lustre */
  TakeTokenFSCall();
  st = fsal_internal_handle2fd(p_context, p_objecthandle, &fd, O_RDWR);
  ReleaseTokenFSCall();
  if(FSAL_IS_ERROR(st))
    ReturnStatus(st, INDEX_FSAL_SetXAttrValue);

  len = strnlen((char *)buffer_addr, buffer_size);
  TakeTokenFSCall();
  if(len == 0)
    rc = fsetxattr(fd, xattr_name->name, "", 1, create ? XATTR_CREATE : XATTR_REPLACE);
  else
    rc = fsetxattr(fd, xattr_name->name, (char *)buffer_addr,
                   len, create ? XATTR_CREATE : XATTR_REPLACE);

  ReleaseTokenFSCall();

  close(fd);

  if(rc != 0)
    Return(posix2fsal_error(errno), errno, INDEX_FSAL_SetXAttrValue);
  else
    Return(ERR_FSAL_NO_ERROR, 0, INDEX_FSAL_SetXAttrValue);
}

fsal_status_t XFSFSAL_SetXAttrValueById(xfsfsal_handle_t * p_objecthandle,      /* IN */
                                        unsigned int xattr_id,  /* IN */
                                        xfsfsal_op_context_t * p_context,       /* IN */
                                        caddr_t buffer_addr,    /* IN */
                                        size_t buffer_size      /* IN */
    )
{
  int rc;
  fsal_status_t st;
  int fd = 0;
  fsal_name_t attr_name;
  char name[FSAL_MAX_NAME_LEN];

  if(attr_is_read_only(xattr_id))
    Return(ERR_FSAL_PERM, 0, INDEX_FSAL_SetXAttrValue);
  else if(xattr_id < XATTR_COUNT)
    /* this is not a UDA (setattr not supported) */
    Return(ERR_FSAL_PERM, 0, INDEX_FSAL_SetXAttrValue);

  /* build fid path in lustre */
  TakeTokenFSCall();
  st = fsal_internal_handle2fd(p_context, p_objecthandle, &fd, O_RDWR);
  ReleaseTokenFSCall();
  if(FSAL_IS_ERROR(st))
    ReturnStatus(st, INDEX_FSAL_SetXAttrValue);

  rc = xattr_id_to_name(fd, xattr_id, name);
  close(fd);
  if(rc)
    Return(rc, errno, INDEX_FSAL_SetXAttrValue);

  FSAL_str2name(name, FSAL_MAX_NAME_LEN, &attr_name);

  return XFSFSAL_SetXAttrValue(p_objecthandle, &attr_name,
                               p_context, buffer_addr, buffer_size, FALSE);
}

/**
 *  Removes a xattr by Id
 *
 * \param p_objecthandle Handle of the object you want to get attribute for.
 * \param p_context pointer to the current security context.
 * \param xattr_id xattr's id
 */
fsal_status_t XFSFSAL_RemoveXAttrById(xfsfsal_handle_t * p_objecthandle,        /* IN */
                                      xfsfsal_op_context_t * p_context, /* IN */
                                      unsigned int xattr_id)    /* IN */
{
  int rc;
  fsal_status_t st;
  int fd = 0;
  char name[FSAL_MAX_NAME_LEN];

  TakeTokenFSCall();
  st = fsal_internal_handle2fd(p_context, p_objecthandle, &fd, O_RDWR);
  ReleaseTokenFSCall();
  if(FSAL_IS_ERROR(st))
    ReturnStatus(st, INDEX_FSAL_SetXAttrValue);

  rc = xattr_id_to_name(fd, xattr_id, name);
  if(rc)
    Return(rc, errno, INDEX_FSAL_SetXAttrValue);

  TakeTokenFSCall();
  rc = fremovexattr(fd, name);
  ReleaseTokenFSCall();

  close(fd);

  if(rc != 0)
    ReturnCode(posix2fsal_error(errno), errno);

  ReturnCode(ERR_FSAL_NO_ERROR, 0);
}                               /* FSAL_RemoveXAttrById */

/**
 *  Removes a xattr by Name
 *
 * \param p_objecthandle Handle of the object you want to get attribute for.
 * \param p_context pointer to the current security context.
 * \param xattr_name xattr's name
 */
fsal_status_t XFSFSAL_RemoveXAttrByName(xfsfsal_handle_t * p_objecthandle,      /* IN */
                                        xfsfsal_op_context_t * p_context,       /* IN */
                                        const fsal_name_t * xattr_name) /* IN */
{
  int rc;
  fsal_status_t st;
  int fd = 0;

  TakeTokenFSCall();
  st = fsal_internal_handle2fd(p_context, p_objecthandle, &fd, O_RDWR);
  ReleaseTokenFSCall();
  if(FSAL_IS_ERROR(st))
    ReturnStatus(st, INDEX_FSAL_SetXAttrValue);

  TakeTokenFSCall();
  rc = fremovexattr(fd, xattr_name->name);
  ReleaseTokenFSCall();

  close(fd);

  if(rc != 0)
    ReturnCode(posix2fsal_error(errno), errno);

  ReturnCode(ERR_FSAL_NO_ERROR, 0);
}                               /* FSAL_RemoveXAttrById */
