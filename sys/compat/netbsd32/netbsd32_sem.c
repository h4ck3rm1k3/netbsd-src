/*	$NetBSD: netbsd32_sem.c,v 1.8 2008/11/14 15:49:20 ad Exp $	*/

/*
 *  Copyright (c) 2006 The NetBSD Foundation.
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *  1. Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 *  ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 *  TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 *  PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 *  BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: netbsd32_sem.c,v 1.8 2008/11/14 15:49:20 ad Exp $");

#ifdef _KERNEL_OPT
#include "opt_posix.h"
#endif

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/dirent.h>
#include <sys/ksem.h>
#include <sys/mount.h>
#include <sys/proc.h>
#include <sys/syscallargs.h>

#include <compat/netbsd32/netbsd32.h>
#include <compat/netbsd32/netbsd32_syscallargs.h>
#include <compat/netbsd32/netbsd32_conv.h>

static int
netbsd32_ksem_copyout(const void *src, void *dst, size_t size)
{
	const intptr_t *idp = src;
	netbsd32_intptr_t id32, *outidp = dst;

	KASSERT(size == sizeof(intptr_t));

	/* Returning a kernel pointer to userspace sucks badly :-( */
	id32 = (netbsd32_intptr_t)*idp;
	return copyout(&id32, outidp, sizeof(id32));
}

int
netbsd32__ksem_init(struct lwp *l, const struct netbsd32__ksem_init_args *uap, register_t *retval)
{
	/* {
		syscallarg(unsigned int) value;
		syscallarg(netbsd32_semidp_t) idp;
	} */

	return do_ksem_init(l, SCARG(uap, value),
	    SCARG_P32(uap, idp), netbsd32_ksem_copyout);
}

int
netbsd32__ksem_open(struct lwp *l, const struct netbsd32__ksem_open_args *uap, register_t *retval)
{
	/* {
		syscallarg(const netbsd32_charp) name;
		syscallarg(int) oflag;
		syscallarg(mode_t) mode;
		syscallarg(unsigned int) value;
		syscallarg(netbsd32_semidp_t) idp;
	} */

	return do_ksem_open(l, SCARG_P32(uap, name),
	    SCARG(uap, oflag), SCARG(uap, mode), SCARG(uap, value),
	    SCARG_P32(uap, idp), netbsd32_ksem_copyout);
}

int
netbsd32__ksem_unlink(struct lwp *l, const struct netbsd32__ksem_unlink_args *uap, register_t *retval)
{
	/* {
		syscallarg(const netbsd32_charp) name;
	} */
	struct sys__ksem_unlink_args ua;

	NETBSD32TOP_UAP(name, const char);
	return sys__ksem_unlink(l, &ua, retval);
}

int
netbsd32__ksem_close(struct lwp *l, const struct netbsd32__ksem_close_args *uap, register_t *retval)
{
	/* {
		syscallarg(netbsd32_intptr_t) id;
	} */
	struct sys__ksem_close_args ua;

	NETBSD32TOX_UAP(id, intptr_t);
	return sys__ksem_close(l, &ua, retval);
}

int
netbsd32__ksem_post(struct lwp *l, const struct netbsd32__ksem_post_args *uap, register_t *retval)
{
	/* {
		syscallarg(netbsd32_intptr_t) id;
	} */
	struct sys__ksem_post_args ua;

	NETBSD32TOX_UAP(id, intptr_t);
	return sys__ksem_post(l, &ua, retval);
}

int
netbsd32__ksem_wait(struct lwp *l, const struct netbsd32__ksem_wait_args *uap, register_t *retval)
{
	/* {
		syscallarg(netbsd32_intptr_t) id;
	} */
	struct sys__ksem_wait_args ua;

	NETBSD32TOX_UAP(id, intptr_t);
	return sys__ksem_wait(l, &ua, retval);
}

int
netbsd32__ksem_trywait(struct lwp *l, const struct netbsd32__ksem_trywait_args *uap, register_t *retval)
{
	/* {
		syscallarg(netbsd32_intptr_t) id;
	} */
	struct sys__ksem_trywait_args ua;

	NETBSD32TOX_UAP(id, intptr_t);
	return sys__ksem_trywait(l, &ua, retval);
}

int
netbsd32__ksem_destroy(struct lwp *l, const struct netbsd32__ksem_destroy_args *uap, register_t *retval)
{
	/* {
		syscallarg(netbsd32_intptr_t) id;
	} */
	struct sys__ksem_destroy_args ua;

	NETBSD32TOX_UAP(id, intptr_t);
	return sys__ksem_destroy(l, &ua, retval);
}

int
netbsd32__ksem_getvalue(struct lwp *l, const struct netbsd32__ksem_getvalue_args *uap, register_t *retval)
{
	/* {
		syscallarg(netbsd32_intptr_t) id;
		syscallarg(netbsd32_intp) value;
	} */
	struct sys__ksem_getvalue_args ua;

	NETBSD32TOX_UAP(id, intptr_t);
	NETBSD32TOP_UAP(value, unsigned int);
	return sys__ksem_getvalue(l, &ua, retval);
}
