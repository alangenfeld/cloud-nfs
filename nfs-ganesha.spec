%define _with_posix 1
%define _with_sharedfsalno 1

Name: nfs-ganesha 
Version: 1.0.1 
Release: 1%{?dist}
Summary: NFS Server running in user space with large cache 
License: LGPL
Group: Applications/System
%if %{?_with_posix:1}%{!?_with_posix:0}
%if %{?_with_postgresql:1}%{!?_with_postgresql:0}
BuildRequires:  postgresql-devel >= 7.1
BuildRequires:  postgresql-libs >= 7.1
%else
BuildRequires:  mysql-devel >= 5.0
BuildRequires:  mysql-libs >= 5.0
%endif
%endif
%if %{?_with_gpfs:1}%{!?_with_gpfs:0}
# we have to be really specific on kernel version
# because we are packaging a module
BuildRequires: kernel == 2.6.18-194.el5
%endif
BuildRequires: flex
BuildRequires: bison
BuildRequires: gcc
BuildRequires: autoconf
BuildRequires: automake
#BuildRequires:  lm_sensors-devel >= 2.8
Url: http://nfs-ganesha.sourceforge.net
Source0: http://downloads.sourceforge.net/nfs-ganesha/%{name}-%{version}.tar.gz
BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)

%description
NFS-GANESHA is a NFS Server running in user space with a large cache.
It comes with various backend modules to support different file systems
and namespaces. This version is compiled to support the posix backend.

%package common
Summary: Common files for all NFS-GANESHA server variants
Group: Applications/System
Provides: %{name} = %{version}-%{release}
Obsoletes: %{name} < 1.0.1 

%description common
NFS-GANESHA is a NFS Server running in user space with a large cache.
It comes with various backend modules to support different file systems
and namespaces. This package contains the common files for all variants.


%if %{?_with_proxy:1}%{!?_with_proxy:0}
%package proxy
Summary: The NFS-GANESHA server compiled as a PROXY 
Group: Applications/System
Requires: %{name}-common = %{version}-%{release}

%description proxy
NFS-GANESHA is a NFS Server running in user space with a large cache.
It comes with various backend modules to support different file systems
and namespaces. This package provides support for NFS-GANESHA as a PROXY.
%endif

%if %{?_with_xfs:1}%{!?_with_xfs:0}
%package xfs
Summary: The NFS-GANESHA server compiled as a NFS gateway to XFS.
Group: Applications/System
Requires: xfsprogs
BuildRequires: xfsprogs-devel xfsprogs-qa-devel
Requires: %{name}-common = %{version}-%{release}

%description xfs
NFS-GANESHA is a NFS Server running in user space with a large cache.
It comes with various backend modules to support different file systems
and namespaces. This package provides support for NFS-GANESHA used on top of XFS.

%endif

%if %{?_with_gpfs:1}%{!?_with_gpfs:0}
%package gpfs
Summary: The NFS-GANESHA server compiled for use with GPFS. 
Group: Applications/System
Requires: %{name}-common = %{version}-%{release}

%description gpfs
NFS-GANESHA is a NFS Server running in user space with a large cache.
It comes with various backend modules to support different file systems
and namespaces. This package provides support for NFS-GANESHA used on top of GPFS.
%endif

%if %{?_with_posix:1}%{!?_with_posix:0}
%package posix
Summary: The NFS-GANESHA server compiled for using the POSIX interface 
Group: Applications/System
%if %{?_with_postgresql:1}%{!?_with_postgresql:0}
Requires: postgresql >= 7.1  postgresql-server >= 7.1
%else
Requires: mysql >= 5.0  mysql-server >= 5.0
%endif
Requires: %{name}-common = %{version}-%{release}

%description posix
NFS-GANESHA is a NFS Server running in user space with a large cache.
It comes with various backend modules to support different file systems
and namespaces. This package provides support for GANESHA using the POSIX
interface.
%endif

%if %{?_with_lustre:1}%{!?_with_lustre:0}
%package lustre
Summary: The NFS-GANESHA server compiled for using the LUSTRE interface 
Group: Applications/System
Requires: %{name}-common = %{version}-%{release}

%description lustre
NFS-GANESHA is a NFS Server running in user space with a large cache.
It comes with various backend modules to support different file systems
and namespaces. This package provides support for GANESHA using the LUSTRE
interface.
%endif

%if %{?_with_dynfsal:1}%{!?_with_dynfsal:0}
%package dynfsal
Summary: The NFS-GANESHA server compiled in a FSAL-les way. 
Group: Applications/System
Requires: %{name}-common = %{version}-%{release}

%description dynfsal
The NFS-GANESHA server compiled in a FSAL-les way. 
This requires the use of FSAL compiled as shared objects. FSALs are loaded dynamically at runtime using the dlopen() function.
%endif 

%if %{?_with_snmp:1}%{!?_with_snmp:0}
%package snmp
Summary: The NFS-GANESHA server compiled as a NFS gateway to SNMP. 
Group: Applications/System
Requires: net-snmp >= 5.1
Requires: lm_sensors-devel >= 2.10
BuildRequires: net-snmp-libs >= 5.1
BuildRequires: net-snmp-devel >= 5.1
Requires: %{name}-common = %{version}-%{release}

%description snmp
NFS-GANESHA is a NFS Server running in user space with a large cache.
It comes with various backend modules to support different file systems
and namespaces. This package provides a NFS gateway to SNMP, making it 
possible to browse SNMP exported information in a procfs-like way.
%endif


%if %{?_with_fuse:1}%{!?_with_fuse:0}
%package fuselike 
Summary: The ganeshaNFS library for FUSE-like bindings
Group: Applications/System
Requires: %{name}-common = %{version}-%{release}

%description fuselike
NFS-GANESHA is a NFS Server running in user space with a large cache.
It comes with various backend modules to support different file systems
and namespaces. This package contains the components for FUSE-like
bindings.

%package fuselike-devel
Summary: The NFS-GANESHA development files for FUSE-like bindings
Group: Applications/System
Requires: pkgconfig
Requires: %{name}-fuselike = %{version}-%{release}

%description fuselike-devel
NFS-GANESHA is a NFS Server running in user space with a large cache.
It comes with various backend modules to support different file systems
and namespaces. This package contains the development headers for FUSE-like
%endif

%if %{?_with_hpss:1}%{!?_with_hpss:0}
%package hpss
Summary: The NFS-GANESHA server compiled as a NFS gateway to HPSS.
Group: Applications/System
Requires: %{name}-common = %{version}-%{release}

%description hpss
NFS-GANESHA is a NFS Server running in user space with a large cache.
It comes with various backend modules to support different file systems
and namespaces. This package provides support for NFS-GANESHA used with HPSS.
%endif


%if %{?_with_zfs:1}%{!?_with_zfs:0}
%package zfs
Summary: The NFS-GANESHA server compiled as a NFS gateway to ZFS.
Group: Applications/System
Requires: libzfswrap
BuildRequires: libzfswrap-devel
Requires: %{name}-common = %{version}-%{release}

%description zfs
NFS-GANESHA is a NFS Server running in user space with a large cache.
It comes with various backend modules to support different file systems
and namespaces. This package provides support for NFS-GANESHA used on top of ZFS.
%endif

%if %{?_with_sharedfsalyes:1}%{!?_with_sharedfsalyes:0}
%if %{?_with_fuse:0}%{!?_with_fuse:1}
%package fsal-posix
Summary: The NFS-GANESHA's FSAL dedicated to posix, provided as a shared object 
Group: Applications/System
Requires: %{name}-common = %{version}-%{release}

%description fsal-posix
This package contains a FSAL library compiled as a shared object. It can be loaded dynamically in a FSAL-less NFS-GANESHA daemon
%endif
%endif

%prep
%setup -q -n %{name}-%{version}

%build

%configure  '--with-fsal=POSIX' '--with-db=MYSQL' 'CC=gcc -g' 'CFLAGS=-g' 'CXXFLAGS=-g'

# try 1 recovery when there is a // make error, then use simple make at 2nd error 
make %{?_smp_mflags}  || make %{?_smp_mflags}  || make


%install
rm -rf $RPM_BUILD_ROOT
mkdir -p $RPM_BUILD_ROOT
mkdir -p $RPM_BUILD_ROOT%{_sysconfdir}/ganesha
mkdir -p $RPM_BUILD_ROOT%{_sysconfdir}/init.d
mkdir -p $RPM_BUILD_ROOT%{_bindir}
mkdir -p $RPM_BUILD_ROOT%{_libdir}
mkdir -p $RPM_BUILD_ROOT%{_libdir}/pkgconfig

make install DESTDIR=$RPM_BUILD_ROOT

install -m 644 config_samples/snmp.conf $RPM_BUILD_ROOT%{_sysconfdir}/ganesha

%if %{?_with_hpss:1}%{!?_with_hpss:0}
install -m 644 config_samples/hpss.ganesha.nfsd.conf           $RPM_BUILD_ROOT%{_sysconfdir}/ganesha
%endif
%if %{?_with_posix:1}%{!?_with_posix:0}
install -m 755 nfs-ganesha.posix.init                          $RPM_BUILD_ROOT%{_sysconfdir}/init.d/nfs-ganesha.posix
install -m 644 config_samples/posix.ganesha.nfsd.conf          $RPM_BUILD_ROOT%{_sysconfdir}/ganesha
install -m 644 FSAL/FSAL_POSIX/DBExt/PGSQL/posixdb_v8.sql      $RPM_BUILD_ROOT%{_sysconfdir}/ganesha
install -m 644 FSAL/FSAL_POSIX/DBExt/PGSQL/posixdb_v7.sql      $RPM_BUILD_ROOT%{_sysconfdir}/ganesha
install -m 644 FSAL/FSAL_POSIX/DBExt/MYSQL/posixdb_mysql5.sql  $RPM_BUILD_ROOT%{_sysconfdir}/ganesha
%endif
%if %{?_with_proxy:1}%{!?_with_proxy:0}
install -m 755 nfs-ganesha.proxy.init                          $RPM_BUILD_ROOT%{_sysconfdir}/init.d/nfs-ganesha.proxy
install -m 644 config_samples/proxy.ganesha.nfsd.conf          $RPM_BUILD_ROOT%{_sysconfdir}/ganesha
%endif
%if %{?_with_xfs:1}%{!?_with_xfs:0}
install -m 755 nfs-ganesha.xfs.init                          $RPM_BUILD_ROOT%{_sysconfdir}/init.d/nfs-ganesha.xfs
install -m 644 config_samples/xfs.ganesha.nfsd.conf          $RPM_BUILD_ROOT%{_sysconfdir}/ganesha
%endif
%if %{?_with_gpfs:1}%{!?_with_gpfs:0}
install -m 755 nfs-ganesha.gpfs.init                          $RPM_BUILD_ROOT%{_sysconfdir}/init.d/nfs-ganesha.gpfs
install -m 644 config_samples/gpfs.ganesha.nfsd.conf          $RPM_BUILD_ROOT%{_sysconfdir}/ganesha
install -m 644 config_samples/gpfs.ganesha.main.conf          $RPM_BUILD_ROOT%{_sysconfdir}/ganesha
install -m 644 config_samples/gpfs.ganesha.exports.conf          $RPM_BUILD_ROOT%{_sysconfdir}/ganesha
rm -f $RPM_BUILD_ROOT/lib/modules/*/modules*
%endif
%if %{?_with_lustre:1}%{!?_with_lustre:0}
install -m 755 nfs-ganesha.lustre.init                          $RPM_BUILD_ROOT%{_sysconfdir}/init.d/nfs-ganesha.lustre
install -m 644 config_samples/lustre.ganesha.nfsd.conf          $RPM_BUILD_ROOT%{_sysconfdir}/ganesha
%endif
%if %{?_with_dynfsal:1}%{!?_with_dynfsal:0}
install -m 755 nfs-ganesha.dynfsal.init                         $RPM_BUILD_ROOT%{_sysconfdir}/init.d/nfs-ganesha.dynfsal
install -m 644 config_samples/dynfsal.ganesha.nfsd.conf         $RPM_BUILD_ROOT%{_sysconfdir}/ganesha
%endif
%if %{?_with_snmp:1}%{!?_with_snmp:0}
install -m 755 nfs-ganesha.snmp.init                           $RPM_BUILD_ROOT%{_sysconfdir}/init.d/nfs-ganesha.snmp
install -m 644 config_samples/snmp.ganesha.nfsd.conf           $RPM_BUILD_ROOT%{_sysconfdir}/ganesha
%endif
%if %{?_with_hpss:1}%{!?_with_hpss:0}
install -m 755 nfs-ganesha.hpss.init                           $RPM_BUILD_ROOT%{_sysconfdir}/init.d/nfs-ganesha.hpss
%endif
%if %{?_with_zfs:1}%{!?_with_zfs:0}
install -m 755 nfs-ganesha.zfs.init                            $RPM_BUILD_ROOT%{_sysconfdir}/init.d/nfs-ganesha.zfs
%endif
install -m 644 config_samples/hosts.ganesha                    $RPM_BUILD_ROOT%{_sysconfdir}/ganesha
install -m 644 config_samples/uidgid_mapfile.ganesha           $RPM_BUILD_ROOT%{_sysconfdir}/ganesha
install -m 755 tools/ganestat.pl                               $RPM_BUILD_ROOT%{_bindir}

%if %{?_with_sharedfsalyes:1}%{!?_with_sharedfsalyes:0}
%if %{?_with_fuse:0}%{!?_with_fuse:1}
rm -f $RPM_BUILD_ROOT%{_libdir}/libfsalposix.la
rm -f $RPM_BUILD_ROOT%{_libdir}/libfsalposix.a
%endif
%endif

mkdir -p  $RPM_BUILD_ROOT%{_datadir}/%{name}
install -m 644 ganesha.vim     $RPM_BUILD_ROOT%{_datadir}/%{name}
install -m 644 ganesha_au.vim  $RPM_BUILD_ROOT%{_datadir}/%{name}
for vimver in 63 64 70 71 ; do
  mkdir -p $RPM_BUILD_ROOT%{_datadir}/vim/vim$vimver/syntax
  mkdir -p $RPM_BUILD_ROOT%{_datadir}/vim/vim$vimver/autoload
  cd $RPM_BUILD_ROOT%{_datadir}/vim/vim$vimver/syntax 
  ln -s ../../../%{name}/ganesha.vim .
  cd $RPM_BUILD_ROOT%{_datadir}/vim/vim$vimver/autoload 
  ln -s ../../../%{name}/ganesha_au.vim .
done


%clean
rm -rf $RPM_BUILD_ROOT

%files common
%defattr(-,root,root,-)
%doc LICENSE.txt Docs/nfs-ganesha-userguide.pdf Docs/nfs-ganesha-adminguide.pdf Docs/nfs-ganeshell-userguide.pdf Docs/nfs-ganesha-ganestat.pdf Docs/using_ganeshell.pdf Docs/nfs-ganesha-convert_fh.pdf Docs/using_*_fsal.pdf
%doc Docs/ganesha_snmp_adm.pdf
%dir %{_sysconfdir}/ganesha/
%dir %{_datadir}/%{name}
%config(noreplace) %{_sysconfdir}/ganesha/hosts.ganesha
%config(noreplace) %{_sysconfdir}/ganesha/snmp.conf
%config(noreplace) %{_sysconfdir}/ganesha/uidgid_mapfile.ganesha
%{_bindir}/ganestat.pl
%{_bindir}/ganesha_log_level
%{_datadir}/%{name}/ganesha.vim
%{_datadir}/%{name}/ganesha_au.vim
%{_datadir}/vim/*/syntax/*
%{_datadir}/vim/*/autoload/*


%if %{?_with_proxy:1}%{!?_with_proxy:0}
%files proxy
%defattr(-,root,root,-)
%{_bindir}/proxy.ganesha.*
%{_bindir}/proxy.ganeshell
%{_sysconfdir}/init.d/nfs-ganesha.proxy
%config %{_sysconfdir}/ganesha/proxy.ganesha*
%endif

%if %{?_with_xfs:1}%{!?_with_xfs:0}
%files xfs
%defattr(-,root,root,-)
%{_bindir}/xfs.ganesha.*
%{_bindir}/xfs.ganeshell
%{_sysconfdir}/init.d/nfs-ganesha.xfs
%config %{_sysconfdir}/ganesha/xfs.ganesha*
%endif

%if %{?_with_gpfs:1}%{!?_with_gpfs:0}
%files gpfs
%defattr(-,root,root,-)
/lib/modules/*/extra/*.ko
%{_bindir}/gpfs.ganesha.*
%{_bindir}/gpfs.ganeshell
%{_sysconfdir}/init.d/nfs-ganesha.gpfs
%config(noreplace) %{_sysconfdir}/ganesha/gpfs.ganesha*
%endif


%if %{?_with_lustre:1}%{!?_with_lustre:0}
%files lustre
%defattr(-,root,root,-)
%{_bindir}/lustre.ganesha.*
%{_bindir}/lustre.ganeshell
%{_sysconfdir}/init.d/nfs-ganesha.lustre
%config %{_sysconfdir}/ganesha/lustre.ganesha*
%endif

%if %{?_with_dynfsal:1}%{!?_with_dynfsal:0}
%files dynfsal
%defattr(-,root,root,-)
%{_bindir}/dynfsal.ganesha.*
%{_bindir}/dynfsal.ganeshell
%{_sysconfdir}/init.d/nfs-ganesha.dynfsal
%config %{_sysconfdir}/ganesha/dynfsal.ganesha*
%endif

%if %{?_with_posix:1}%{!?_with_posix:0}
%files posix 
%defattr(-,root,root,-)
%{_bindir}/posix.ganesha.*
%{_bindir}/posix.ganeshell
%{_sysconfdir}/init.d/nfs-ganesha.posix
%config(noreplace) %{_sysconfdir}/ganesha/posixdb_v8.sql 
%config(noreplace) %{_sysconfdir}/ganesha/posixdb_mysql5.sql 
%config(noreplace) %{_sysconfdir}/ganesha/posixdb_v7.sql 
%config(noreplace) %{_sysconfdir}/ganesha/posix.ganesha* 
%endif


%if %{?_with_snmp:1}%{!?_with_snmp:0}
%files snmp
%defattr(-,root,root,-)
%{_bindir}/snmp.ganesha.*
%{_bindir}/snmp.ganeshell
%{_sysconfdir}/init.d/nfs-ganesha.snmp
%config(noreplace) %{_sysconfdir}/ganesha/snmp.ganesha*
%endif

%if %{?_with_fuse:1}%{!?_with_fuse:0}
%files fuselike 
%defattr(-,root,root,-)
%{_libdir}/libganeshaNFS.la
%{_libdir}/libganeshaNFS.a
%{_libdir}/libganeshaNFS.so.*
%{_bindir}/fuse.ganesha.convertFH


%files fuselike-devel
%defattr(-,root,root,-)
%{_libdir}/pkgconfig/libganeshaNFS.pc
%{_libdir}/libganeshaNFS.a
%{_libdir}/libganeshaNFS.so
%{_includedir}/ganesha_fuse_wrap.h
%endif


%if %{?_with_hpss:1}%{!?_with_hpss:0}
%files hpss
%defattr(-,root,root,-)
%{_bindir}/hpss.ganesha.*
%{_bindir}/hpss.ganeshell
%{_sysconfdir}/init.d/nfs-ganesha.hpss
%config(noreplace) %{_sysconfdir}/ganesha/hpss.ganesha*
%endif

%if %{?_with_zfs:1}%{!?_with_zfs:0}
%files zfs
%defattr(-,root,root,-)
%{_bindir}/zfs.ganesha.*
%{_bindir}/zfs.ganeshell
%{_sysconfdir}/init.d/nfs-ganesha.zfs
%endif

%if %{?_with_sharedfsalyes:1}%{!?_with_sharedfsalyes:0}
%if %{?_with_fuse:0}%{!?_with_fuse:1}
%files fsal-posix
%defattr(-,root,root,-)
%{_libdir}/libfsalposix.so*
%endif
%endif


%if %{?_with_proxy:1}%{!?_with_proxy:0}
%post proxy
chkconfig --add nfs-ganesha.proxy
%endif

%if %{?_with_xfs:1}%{!?_with_xfs:0}
%post xfs
chkconfig --add nfs-ganesha.xfs
%endif

%if %{?_with_gpfs:1}%{!?_with_gpfs:0}
%post gpfs
chkconfig --add nfs-ganesha.gpfs
/sbin/depmod
%endif

%if %{?_with_lustre:1}%{!?_with_lustre:0}
%post lustre
chkconfig --add nfs-ganesha.lustre
%endif

%if %{?_with_posix:1}%{!?_with_posix:0}
%post posix
chkconfig --add nfs-ganesha.posix
%endif

%if %{?_with_snmp:1}%{!?_with_snmp:0}
%post snmp
chkconfig --add nfs-ganesha.snmp
%endif

%if %{?_with_hpss:1}%{!?_with_hpss:0}
%post
%endif

%if %{?_with_zfs:1}%{!?_with_zfs:0}
%post zfs
chkconfig --add nfs-ganesha.zfs
%endif

%if %{?_with_fuse:1}%{!?_with_fuse:0}
%post
%endif

%postun 


%changelog
* Fri Jun 25  2010 Philippe Deniel <philippe.deniel@cea.fr> 0.99.66-1
- FSAL_XFS now has lock support
- Brand new FSAL_GPFS added (patch from IBM) to natively support GPFS
- FSAL_POSIX and FSAL_XFS now have quota support (via rquota v1/v2 protocol and the use of the quotactl function)
- Typos fixed in doxygen.conf files
- FSAL_TEMPLATE updated (had new functions for quota and lock management)
- pNFS/LAYOUT_FILES works with multiple Data Server

* Tue Jun  8  2010 Philippe Deniel <philippe.deniel@cea.fr> 0.99.65-1
- New FSAL_XFS designed for natively exporting XFS filesystems
- Integration of a patch from Aneesh Kumar that implements Async NLM and NSM support
- A patch from Frank Filz related to POSIX behavior when opening file
- add '--enable-ds' in configure to configure nfs-ganesha as a NFSv4.1 server usable as a pNFS Data Server
- FSAL_LUSTRE : add lockdesc support
- Bug Fix: nfs4_op_access was not managing secondary groups properly. It now relyes on the FSAL for this.

* Thu Apr 29  2010 Philippe Deniel <philippe.deniel@cea.fr> 0.99.64-1
- RPM Packaging : add chkconfig --add in %post
- Export Access Type "MDONLY" was not managed when using NFSv4
- Add safety check to cache_inode_remove/cache_inode_create and cache_inode_link to prevent from non allowed access.
- Statistics for NFSv4.0 and NFSv4.1 operations have been added
- Bug Fix: default value for FSINFO3::dtpref was 0. Value 16384 is now used.
- Bug Fix: OPEN4 returns NFS4ERR_ROFS when used from the pseudofs
- Early (unstable) implementation of pNFS provided. Will continue and be stabilized in later releases.
- Project is now released under the terms of the LGPLv3

* Thu Mar 25  2010 Philippe Deniel <philippe.deniel@cea.fr> 0.99.63-1
- A big patch provided by Aneesh Kumar (aneesh.kumar@linux.vnet.ibm.com) implements the NLMv4 protocol
- A "indent" target has been add to the src/Makefile.am . 
- C-format style template for emacs provided by Sean Dague (japh@us.ibm.com)
- Bug fix (Frank Filz) : readdir had an extraneous empty request with eod=TRUE
- Bug fix : It was impossible to mount an exported entry's sub directory

* Fri Mar  5  2010 Philippe Deniel <philippe.deniel@cea.fr> 0.99.62-1
- Security fix : badly managed caller_gid in nfs_exports.c
- Fixed a typo in nfs-ganesha.spec.in
- RPM packaging : fixed bad dependences for db engine to be used with FSAL_POSIX
- Debian Packaging : fixed same dep problems as above with rpm files
- Bug Fix : in idmapper.c, functions utf82uid and utf82gid were badly managing parameters when used with libnfsidmap
- pNFS implementation : now support attribute FATTR4_FS_LAYOUT_TYPE
- Fixed a bug in a Makefile.am that prevent target 'check' to compile
- RPM packaging : when compiling rpm files, only those related to the FSAL chosen at ./configure time are build
- fixed two typos and one potential memleaks (thanks to IBM guys how located this in the code)

* Fri Jan 22  2010 Philippe Deniel <philippe.deniel@cea.fr> 0.99.61-1
- A patch from Eric Sesterhenn about memleaks has been integrated.
- Bug Fix : now check value of csa_flags for OP4_CREATE_SESSION
- Bug Fix : OP4_LOOKUPP should return NFS4ERR_SYMLINK instead of NFS4ERR_NOTDIR when cfh is related to a symbolic link.
- Bug Fix : error NFS4ERR_NOT_ONLY_OP managed for OP4_EXCHANGE_ID
- Bug Fix : OP4_LOOKUPP should return NFS4ERR_NOENT when called from the rootfh
- Bug Fix : management of NFS4ERR_NOT_ONLY_OP introduced a bug when compiling without NFSv4.1 support. This is now fixed.
- Changed bad #define in Log/log_functions.c (former situation could lead to possible buffer overflow)
- A patch by Erik Levinson about the use of libnfsidmap with gssrpc has been integrated
- Bug Fix : it was impossible to compile with both support for gssrpc and support for NFSv4.1 (mismatch in nfsv41.h and xdr_nfsv41.c)

* Mon Nov 30  2009 Philippe Deniel <philippe.deniel@cea.fr> 0.99.60-1
- The uid/gid mapping functions did a bad use of their related idmapper_cache functions
- Bug Fix : with kernel newer than 2.6.29, Connectathon's test6 failed
- Lock supports is available and apparently healthy with NFSv4.1
- Bug Fix: NFSv4 rsize/wsize had always value 1024 that killed performances
- Bug Fix : in nfsv4, the same open_owner opening a previously opened  fileid did not get the same stateid.
- Bug Fix : most of the time, files opened/created via NFSv4 were never closed

* Fri Sep 14  2009 Philippe Deniel <philippe.deniel@cea.fr> 0.99.58-1
- Early implementation of NFSv4.1 added 
- Add use of libnfsidmap

* Thu Jul 30  2009 Philippe Deniel <philippe.deniel@cea.fr> 0.99.57-1
- Add write/commit logic to NFSv3 / NFSv4 
- Fix many bugs related to clientid/open_owner/lockonwner/seqid

* Thu Jul  9 2009 Philippe Deniel <philippe.deniel@cea.fr> 0.99.56-1
- Change two debug messages in MFSL_ASYNC
- Removed a debug messages in fusexmp_fh
- Bug fix in RW_Lock (may lead to deadlock when used in parralel with several clients
- Prevent FSAL_PROXY to use udp as a transport layer
- MFSL_ASYNC: now, only root can chmod or chgrp on a file/dir/symlink
- MFSL_ASYNC: the way mfsl_async_symlink is work was fully reviewd

* Thu Jun 22 2009 Philippe Deniel <philippe.deniel@cea.fr> 0.99.55-1
- Bug fixed in nfs4_op_open (bad allocation)
- For compatibility reason with older clients, a "rather stateless" implementation of the NFSv4 state model was set my default.
- Regular stateid model (still in progress) is still available as a ./configure argument.

* Thu May 28 2009 Philippe Deniel <philippe.deniel@cea.fr> 0.99.54-1
- Many bugs fixed in NFSv4 locks and states management

* Wed May 20 2009 Philippe Deniel <philippe.deniel@cea.fr> 0.99.53-1
- Many bugs fixed in MFSL_ASYNC (for FSAL_PROXY)
- FSAL_POSIX now uses file descriptor and pread/pwrite instead of FILE* and fseek
- NFSv4 implementation now supports NFSv4 referrals

* Mon Apr  4 2009 Philippe Deniel <philippe.deniel@cea.fr> 0.99.52-1
- Several Bug fixes
- MFSL_ASYNC uses preallocated entries in a cleaner way
- Beta version of FSAL_LUSTRE for LUSTREv2
- Several I/O optimisation

* Tue Mar  3 2009 Philippe Deniel <philippe.deniel@cea.fr> 0.99.51-1
- Bug Fixes
- FSAL_POSIX has been ported to MySQL. 
- MFSL_ASYNC has been added for unlink/link/rename/remove/setattr/mkdir/create

* Fri Jan 23 2009 Philippe Deniel <philippe.deniel@cea.fr> 0.99.50-1
- several bugs fixes
- packages for non-redhat platform

* Tue Dec 23 2008 Philippe Deniel <philippe.deniel@cea.fr> 0.99.49-1
- first release of year 2009
- product with all available FSALs has been ported to BSD 7.0 and Linux ia64

* Mon Dec 15 2008 Philippe Deniel <philippe.deniel@cea.fr> 0.99.48-1
- Renamed yyparse and yylex functions so that GANESHA/FUSE would no more interfer with FUSE modules that uses lex/yacc parsing
- FSAL_PROXY was ported to MacOS X (Darwin 9.5.0)
- FSAL_FUSE was ported to MacOS X (Darwin 9.5.0). This allow userspace libs with fuse binding to be used from MacOS through NFS-GANESHA
- FSAL_SNMP has been ported to MacOS X (Darwin 9.5.0)
- FSAL_POSIX has been ported to MacOS X (Darwin 9.5.0)

* Thu Dec  4 2008 Philippe Deniel <philippe.deniel@cea.fr> 0.99.47-1
- Two unprecisions in NFSv3 coding were found with Spec NFS bench, they were fixed
- Code cleanup : removed structure related to deprecated way of making datacache's GC
- Added a new module name MFSL (will provide additionnal features like md writeback in later versions)

* Thu Nov 18 2008 Philippe Deniel <philippe.deniel@cea.fr> 0.99.46-1
- Bug Fix in tcp dispatcher 
- use of RW_lock inside md cache
- use external command to perform datacache gc 

* Mon Nov  5 2008 Philippe Deniel <philippe.deniel@cea.fr> 0.99.45-1
- Add IPv6 support via TIRPC
- Several bug fixed

* Mon Oct 20 2008 Philippe Deniel <philippe.deniel@cea.fr> 0.99.44-5
- Add scripts to be installed in /etc/init.d to start nfs-ganesha as a service
- Add "vim files" to the common rpm

* Wed Oct 15 2008 Philippe Deniel <philippe.deniel@cea.fr> 0.99.44-4
- Add TIRPC support as possible replacement for ONCRPC (later release will support IPv6)

* Wed Oct 15 2008 Thomas Leibovici <thomas.leibovici@cea.fr> 0.99.44-3
- Compatibility stuff for FUSE filesystems that use getdir() instead of readdir().
- FUSE-like binding now support filesystems with no inode numbers.

* Wed Oct 15 2008 Philippe Deniel <philippe.deniel@cea.fr> 0.99.44-2
- Add libganeshaNFS.pc so that to provide with pkgconfig support 

* Tue Oct 14 2008 Philippe Deniel <philippe.deniel@cea.fr> 0.99.44-1
- Many bug fixed in nfs4_op_lock.c
- bug fixed in nfs4_op_open (multiple stateid for the same open_owner)

* Mon Oct  9 2008 Philippe Deniel <philippe.deniel@cea.fr> 0.99.43-3
- libganeshaNFS is available as both static and shared libraries

* Wed Oct  8 2008 Tom "spot" Callaway <tcallawa@redhat.com> 0.99.43-2
- more cleanups

* Mon Oct  6 2008 Philippe Deniel <philippe.deniel@cea.fr> 0.99.43-1
- NodeSets syntax can now be used in the configuration file 

* Mon Oct  6 2008 Tom "spot" Callaway <tcallawa@redhat.com> 0.99.43-1
- first pass at a Fedora package

* Mon Oct  6 2008 Philippe Deniel <philippe.deniel@cea.fr> 0.99.43-1
- Added RPCSEC_GSS suppprt for NFSv3 and NFSv4 (with uidgid mapper enhancements)

* Fri Oct  3 2008 Philippe Deniel <philippe.deniel@cea.fr> 0.99.43-1
- Fixed issues with RPCSEC_GSS. This now work with krb5, krb5i and krb5p

* Mon Sep 29 2008 Philippe Deniel <philippe.deniel@cea.fr> 0.99.42-1
- Added xattr support in NFSv4. For object "foo" a ghost
  directory name ".xattr.d.foo" is used to access extended attributes
- Added xattr ghost directory and ghost objects for NFSv3. These "extended
 attributes" are read-only for the moment

* Mon Aug 18 2008 Philippe Deniel <philippe.deniel@cea.fr> 0.99.41-1
- Fixed nfs4_op_access bug due to bad interpretation of the RFC for OP_ACCESS4
- Added extended features in BuddyMalloc module to enable extended memory leak tracking
- Fixed a bug in FSAL_PROXY that made every user have root permissions  in a few situations
- Bug fixed: bad offset management in FSAL_read/FSAL_write for FSAL_PROXY.
- Use All-0 stateid for r/w operations made for maintaining the data
- the parameter NFSv4_Proxy::Open_by_FH_Working_Dir is no more required for configuring FSAL_PROXY
- FSAL_PROXY now supports RPCSEC_GSS authentication

* Thu Jul 24 2008 Philippe Deniel <philippe.deniel@cea.fr> 0.99.41-1
- Fixed many memory allocation errors in FSAL_PROXY and NFSv4 implemtation with efence
- Added Handle Mapping feature in FSAL_PROXY which makes it possible to export
  back in NFSv2 and NFSv3 from a proxyfied server accessed via NFSv4

* Wed Jul  7 2008 Philippe Deniel <philippe.deniel@cea.fr> 0.99.40-1
- Lock support in NFSv4 (alpha version)

* Wed Jul  2 2008 Philippe Deniel <philippe.deniel@cea.fr> 0.99.39-1
- Update dependies

* Wed Jul  2 2008 Philippe Deniel <philippe.deniel@cea.fr> 0.99.38-1
- Added filehandle conversion utility documentation

* Wed Jul  2 2008 Philippe Deniel <philippe.deniel@cea.fr> 0.99.38-1
- Added filehandle conversion utility (*.ganesha.convert_fh)

* Wed Jul  2 2008 Thomas Leibovici <thomas.leibovici@cea.fr> 0.99.37-1
- Added conditionnal snmp_adm

* Tue Jul  1 2008 Thomas Leibovici <thomas.leibovici@cea.fr> 0.99.36-1
- Added extra documentation to packages 

* Thu Jun  5 2008 Philippe Deniel <philippe.deniel@cea.fr> 0.99.35-1
- Added ganestat.pl to the different packages 

* Wed Apr 16 2008 Philippe Deniel <philippe.deniel@cea.fr> 0.99.34-1
- Added sub-packages for hpss, with conditionnals

* Tue Apr  4 2008 Philippe Deniel <philippe.deniel@cea.fr> 0.99.33-1
- Added sub-packages

* Mon Mar 31 2008 Philippe Deniel <philippe.deniel@cea.fr> 0.99.32-1
- the rpmbuild specfile now generates all of the binaries

* Tue Mar 18 2008 Philippe Deniel <philippe.deniel@cea.fr> 0.99.31-1
- add configuration template files

* Fri Feb 22 2008 Philippe Deniel <philippe.deniel@cea.fr> 0.99.30-1
- add spec file

