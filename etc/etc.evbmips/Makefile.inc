#	$NetBSD: Makefile.inc,v 1.12 2009/12/14 01:12:05 matt Exp $
#
#	etc.evbmips/Makefile.inc -- evbmips-specific etc Makefile targets
#

# If you change the list of distributed kernels, don't forget
# to update the release documentation in distrib/notes/common/contents

KERNEL_SUFFIXES=	srec

.if ${MACHINE_ARCH} == "mipsel"
KERNEL_SETS=		ALCHEMY DBAU1500 DBAU1550 MALTA MTX-1 OMSAL400
BUILD_KERNELS=		INSTALL_OMSAL400 INSTALL_MALTA

.elif ${MACHINE_ARCH} == "mips64eb" || ${MACHINE_ARCH} == "mips64el"
KERNEL_SETS=		MALTA32 MALTA64
BUILD_KERNELS=		INSTALL_MALTA32 INSTALL_MALTA64

.else
KERNEL_SETS=		ALCHEMY DBAU1500 DBAU1550 MALTA
BUILD_KERNELS=		INSTALL_OMSAL400 INSTALL_MALTA

.endif
