/*	$NetBSD: rumpuser_int.h,v 1.3 2010/05/18 14:58:41 pooka Exp $	*/

/*
 * Copyright (c) 2008 Antti Kantee.  All Rights Reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <stdlib.h>

#include <rump/rumpuser.h>

extern kernel_lockfn rumpuser__klock;
extern kernel_unlockfn rumpuser__kunlock;
extern int rumpuser__wantthreads;

#define KLOCK_WRAP(a)							\
do {									\
	int nlocks;							\
	rumpuser__kunlock(0, &nlocks, NULL);				\
	a;								\
	rumpuser__klock(nlocks, NULL);					\
} while (/*CONSTCOND*/0)

#define DOCALL(rvtype, call)						\
{									\
	rvtype rv;							\
	rv = call;							\
	if (rv == -1)							\
		*error = errno;						\
	else								\
		*error = 0;						\
	return rv;							\
}

#define DOCALL_KLOCK(rvtype, call)					\
{									\
	rvtype rv;							\
	int nlocks;							\
	rumpuser__kunlock(0, &nlocks, NULL);				\
	rv = call;							\
	rumpuser__klock(nlocks, NULL);					\
	if (rv == -1)							\
		*error = errno;						\
	else								\
		*error = 0;						\
	return rv;							\
}