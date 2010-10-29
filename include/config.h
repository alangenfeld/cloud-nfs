/* include/config.h.  Generated from config.h.in by configure.  */
/* include/config.h.in.  Generated from configure.ac by autoheader.  */

/* Define if building universal (internal helper macro) */
/* #undef AC_APPLE_UNIVERSAL_BUILD */

/* Build Operating System is FreeBSD */
/* #undef APPLE */

/* Big endian system */
/* #undef BIGEND */

/* Build Operating System is FreeBSD */
/* #undef FREEBSD */

/* Name of the filesystem */
#define FS_NAME "posix"

/* Define to 1 if you have the <dirent.h> header file, and it defines `DIR'.
   */
#define HAVE_DIRENT_H 1

/* Define to 1 if you have the <dlfcn.h> header file. */
#define HAVE_DLFCN_H 1

/* Define to 1 if you have the <inttypes.h> header file. */
#define HAVE_INTTYPES_H 1

/* krb5 library is available */
/* #undef HAVE_KRB5 */

/* Define to 1 if you have the `c' library (-lc). */
#define HAVE_LIBC 1

/* Define to 1 if you have the `curses' library (-lcurses). */
/* #undef HAVE_LIBCURSES */

/* Define to 1 if you have the `pthread' library (-lpthread). */
#define HAVE_LIBPTHREAD 1

/* Define to 1 if you have the `readline' library (-lreadline). */
/* #undef HAVE_LIBREADLINE */

/* Define to 1 if you have the <memory.h> header file. */
#define HAVE_MEMORY_H 1

/* Define to 1 if you have the <ndir.h> header file, and it defines `DIR'. */
/* #undef HAVE_NDIR_H */

/* Define to 1 if the system has the type `ptrdiff_t'. */
#define HAVE_PTRDIFF_T 1

/* Define to 1 if stdbool.h conforms to C99. */
#define HAVE_STDBOOL_H 1

/* Define to 1 if you have the <stdint.h> header file. */
#define HAVE_STDINT_H 1

/* Define to 1 if you have the <stdlib.h> header file. */
#define HAVE_STDLIB_H 1

/* Define to 1 if you have the <strings.h> header file. */
#define HAVE_STRINGS_H 1

/* Define to 1 if you have the <string.h> header file. */
#define HAVE_STRING_H 1

/* Define to 1 if `st_blksize' is member of `struct stat'. */
#define HAVE_STRUCT_STAT_ST_BLKSIZE 1

/* Define to 1 if `st_blocks' is member of `struct stat'. */
#define HAVE_STRUCT_STAT_ST_BLOCKS 1

/* Define to 1 if `st_rdev' is member of `struct stat'. */
#define HAVE_STRUCT_STAT_ST_RDEV 1

/* Define to 1 if your `struct stat' has `st_blocks'. Deprecated, use
   `HAVE_STRUCT_STAT_ST_BLOCKS' instead. */
#define HAVE_ST_BLOCKS 1

/* Define to 1 if you have the <sys/dir.h> header file, and it defines `DIR'.
   */
/* #undef HAVE_SYS_DIR_H */

/* Define to 1 if you have the <sys/ndir.h> header file, and it defines `DIR'.
   */
/* #undef HAVE_SYS_NDIR_H */

/* Define to 1 if you have the <sys/stat.h> header file. */
#define HAVE_SYS_STAT_H 1

/* Define to 1 if you have the <sys/types.h> header file. */
#define HAVE_SYS_TYPES_H 1

/* Define to 1 if you have the <sys/uio.h> header file. */
#define HAVE_SYS_UIO_H 1

/* Define to 1 if you have the <unistd.h> header file. */
#define HAVE_UNISTD_H 1

/* Define to 1 if you have the <xfs/handle.h> header file. */
/* #undef HAVE_XFS_HANDLE_H */

/* Define to 1 if you have the <xfs/xfs.h> header file. */
/* #undef HAVE_XFS_XFS_H */

/* Define to 1 if the system has the type `_Bool'. */
#define HAVE__BOOL 1

/* Build Operating System is Linux */
#define LINUX 1

/* Little endian system */
#define LITTLEEND 1

/* Define to the sub-directory in which libtool stores uninstalled libraries.
   */
#define LT_OBJDIR ".libs/"

/* Mount list is disabled */
#define NOMNTLIST 1

/* krb5 library is not available */
#define NO_KRB5 1

/* Name of package */
#define PACKAGE "nfs-ganesha"

/* Define to the address where bug reports for this package should be sent. */
#define PACKAGE_BUGREPORT "philippe.deniel@cea.fr,thomas.leibovici@cea.fr "

/* Define to the full name of this package. */
#define PACKAGE_NAME "nfs-ganesha"

/* Define to the full name and version of this package. */
#define PACKAGE_STRING "nfs-ganesha 1.0.1"

/* Define to the one symbol short name of this package. */
#define PACKAGE_TARNAME "nfs-ganesha"

/* Define to the version of this package. */
#define PACKAGE_VERSION "1.0.1"

/* The size of `long', as computed by sizeof. */
#define SIZEOF_LONG 4

/* Build Operating System is Solaris */
/* #undef SOLARIS */

/* Define to 1 if you have the ANSI C header files. */
#define STDC_HEADERS 1

/* Define to 1 if you can safely include both <sys/time.h> and <time.h>. */
#define TIME_WITH_SYS_TIME 1

/* Define to 1 if your <sys/time.h> declares `struct tm'. */
/* #undef TM_IN_SYS_TIME */

/* Enable extensions on AIX 3, Interix.  */
#ifndef _ALL_SOURCE
# define _ALL_SOURCE 1
#endif
/* Enable GNU extensions on systems that have them.  */
#ifndef _GNU_SOURCE
# define _GNU_SOURCE 1
#endif
/* Enable threading extensions on Solaris.  */
#ifndef _POSIX_PTHREAD_SEMANTICS
# define _POSIX_PTHREAD_SEMANTICS 1
#endif
/* Enable extensions on HP NonStop.  */
#ifndef _TANDEM_SOURCE
# define _TANDEM_SOURCE 1
#endif
/* Enable general extensions on Solaris.  */
#ifndef __EXTENSIONS__
# define __EXTENSIONS__ 1
#endif


/* Version number of package */
#define VERSION "1.0.1"

/* No Comment */
#define VERSION_COMMENT "GANESHA 64 bits compliant. SNMP exported stats. FSAL_PROXY re-exports NFSv3. RPCSEC_GSS support (partial). FUSELIKE added"

/* Define WORDS_BIGENDIAN to 1 if your processor stores words with the most
   significant byte first (like Motorola and SPARC, unlike Intel). */
#if defined AC_APPLE_UNIVERSAL_BUILD
# if defined __BIG_ENDIAN__
#  define WORDS_BIGENDIAN 1
# endif
#else
# ifndef WORDS_BIGENDIAN
/* #  undef WORDS_BIGENDIAN */
# endif
#endif

/* Define to 1 if `lex' declares `yytext' as a `char *' by default, not a
   `char[]'. */
#define YYTEXT_POINTER 1

/* Build Operating System is FreeBSD */
/* #undef _APPLE */

/* Build shared FSAL */
/* #undef _BUILD_SHARED_FSAL */

/* Using XDR extended types */
#define _EXTENDED_TYPE_NEEDED 1

/* Number of bits in a file offset, on hosts where this is settable. */
#define _FILE_OFFSET_BITS 64

/* Build Operating System is FreeBSD */
/* #undef _FREEBSD */

/* enable GSSAPI support */
/* #undef _HAVE_GSSAPI */

/* enable GSSRPC support */
/* #undef _HAVE_GSSRPC */

/* enable TIRPC support */
/* #undef _HAVE_TIRPC */

/* Large file support */
/* #undef _LARGEFILE64_SOURCE */

/* Define for large files, on AIX-style hosts. */
/* #undef _LARGE_FILES */

/* Build Operating System is Linux */
#define _LINUX 1

/* Define to 1 if on MINIX. */
/* #undef _MINIX */

/* Don't use MOUNT PROTOCOL's client list */
#define _NO_MOUNT_LIST 1

/* Using PostgreSQL version 8.x */
/* #undef _PGSQL8 */

/* Define to 2 if the system does not provide POSIX.1 features except with
   this defined. */
/* #undef _POSIX_1_SOURCE */

/* Define to 1 if you need to in order for `stat' and other things to work. */
/* #undef _POSIX_SOURCE */

/* export GANESHA statistics with SNMP */
/* #undef _SNMP_ADM_ACTIVE */

/* Build Operating System is Solaris */
/* #undef _SOLARIS */

/* enable pNFS DS support */
/* #undef _USE_DS */

/* GANESHA is compiled as a FUSE-like library */
/* #undef _USE_FUSE */

/* GANESHA is compiled with GHOST FS FSAL */
/* #undef _USE_GHOSTFS */

/* GANESHA exports GPFS Filesystem */
/* #undef _USE_GPFS */

/* enable security management with GSSRPC */
/* #undef _USE_GSSRPC */

/* GANESHA is compiled with HPSS FSAL */
/* #undef _USE_HPSS */

/* GANESHA exports Lustre Filesystem */
/* #undef _USE_LUSTRE */

/* Use MFSL module */
/* #undef _USE_MFSL */

/* Use MFSL_ASYNC module which calls the FSAL functions asynchronously */
/* #undef _USE_MFSL_ASYNC */

/* Use MFSL_NULL module which calls directly the FSAL functions */
/* #undef _USE_MFSL_NULL */

/* Use MFSL_PROXY_RPCSECGSS, wraps the FSAL call for RPCSECGSS client side in
   proxy */
/* #undef _USE_MFSL_PROXY_RPCSECGSS */

/* Using MySQL database */
#define _USE_MYSQL 1

/* Use NFSv4.0 */
#define _USE_NFS4_0 1

/* Use NFSv4.1 */
/* #undef _USE_NFS4_1 */

/* enable Id Mapping through libnfsidmap */
/* #undef _USE_NFSIDMAP */

/* enable NFSv4 support */
/* #undef _USE_NLM */

/* Using PostgreSQL database */
/* #undef _USE_PGSQL */

/* enable NFSv4.1/pNFS support */
/* #undef _USE_PNFS */

/* GANESHA is compiled with POSIX FSAL */
#define _USE_POSIX 1

/* GANESHA is compiled with NFS PROXY FSAL */
/* #undef _USE_PROXY */

/* enable quota support */
/* #undef _USE_QUOTA */

/* enable shared FSAL */
/* #undef _USE_SHARED_FSAL */

/* GANESHA is compiled with SNMP FSAL */
/* #undef _USE_SNMP */

/* enable TIRPC support */
/* #undef _USE_TIRPC */

/* enable IPv6 support via TIRPC */
/* #undef _USE_TIRPC_IPV6 */

/* GANESHA exports XFS Filesystem */
/* #undef _USE_XFS */

/* GANESHA exports ZFS Filesystem */
/* #undef _USE_ZFS */

/* Compiling with _XOPEN_SOURCE functions */
/* #undef _XOPEN_SOURCE */

/* Define missing types and 64bits functions on Solaris */
#define __EXTENSIONS__ 1

/* Define to empty if `const' does not conform to ANSI C. */
/* #undef const */

/* Define to `int' if <sys/types.h> doesn't define. */
/* #undef gid_t */

/* Define to `__inline__' or `__inline' if that's what the C compiler
   calls it, or to nothing if 'inline' is not supported under any name.  */
#ifndef __cplusplus
/* #undef inline */
#endif

/* Define to `int' if <sys/types.h> does not define. */
/* #undef mode_t */

/* Define to `long int' if <sys/types.h> does not define. */
/* #undef off_t */

/* Define to `int' if <sys/types.h> does not define. */
/* #undef pid_t */

/* Define to `unsigned int' if <sys/types.h> does not define. */
/* #undef size_t */

/* Define to `int' if <sys/types.h> doesn't define. */
/* #undef uid_t */

/* Define to empty if the keyword `volatile' does not work. Warning: valid
   code using `volatile' can become incorrect without. Disable with care. */
/* #undef volatile */
