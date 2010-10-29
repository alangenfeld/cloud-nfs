
#
# This macro test for rpcgen 
#
# GA_PROG_RPCGEN
#
AC_DEFUN([GA_PROG_RPCGEN],
[
        # looking for rpcgen program
        AC_CHECK_PROGS(RPCGEN, rpcgen, rpcgen)
])

#
# This macro test for 'net-snmp-config' command 
#
# GA_PROG_NETSNMP_CONFIG
#
AC_DEFUN([GA_PROG_NETSNMP_CONFIG],
[
        # looking for net-snmp-config program
        AC_CHECK_PROGS(NETSNMP_CONFIG, net-snmp-config)
])

#
# This macro test for doxygen
#
# GA_PROG_DOXYGEN
#
AC_DEFUN([GA_PROG_DOXYGEN],
[
        # looking for doxygen program
        AC_CHECK_PROGS(DOXYGEN, doxygen)
])

