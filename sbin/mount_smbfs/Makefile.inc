#	$NetBSD: Makefile.inc,v 1.3 2010/04/03 11:55:41 plunky Exp $
#

SRCS+=	rcfile.c ctx.c cfopt.c subr.c nls.c rap.c mbuf.c rq.c file.c \
	print.c kiconv.c nb.c nb_name.c nb_net.c nbns_rq.c

SMBDIST=${NETBSDSRCDIR}/dist/smbfs

CPPFLAGS+= -I${SMBDIST}/include -I${SMBDIST}/mount_smbfs

.PATH:	${SMBDIST}/mount_smbfs ${SMBDIST}/lib/smb

DPADD+=${LIBUTIL}
LDADD+=-lutil

.if (defined(HAVE_GCC) && ${HAVE_GCC} == 4) || defined(HAVE_PCC)
.for f in ctx mbuf nb_name nbns_rq
COPTS.${f}.c+=       -Wno-pointer-sign
.endfor
.endif