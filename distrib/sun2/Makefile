#	$NetBSD: Makefile,v 1.9 2008/06/23 02:16:54 matt Exp $

SUBDIR=		ramdisk miniroot cdroms
TARGETS+=	release

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/common/Makefile.distrib"

release: check_RELEASEDIR .WAIT \
	    ${DESTDIR}/usr/mdec/bootyy ${DESTDIR}/usr/mdec/netboot \
	    ${DESTDIR}/usr/mdec/tapeboot MakeBootTape MakeInstTape
	${RELEASE_INSTALL} \
	    ${DESTDIR}/usr/mdec/bootyy ${DESTDIR}/usr/mdec/netboot \
	    ${RELEASEDIR}/${RELEASEMACHINEDIR}/installation/netboot
	${RELEASE_INSTALL} ${DESTDIR}/usr/mdec/tapeboot \
	    ${.CURDIR}/MakeBootTape ${.CURDIR}/MakeInstTape \
	    ${RELEASEDIR}/${RELEASEMACHINEDIR}/installation/tapeimage

iso_image:
	${MAKEDIRTARGET} cdroms iso_image

.include <bsd.subdir.mk>
