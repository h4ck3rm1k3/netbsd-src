/*	$NetBSD: main.c,v 1.10 2010/03/19 16:25:33 pooka Exp $	*/

/*-
 * Copyright (c) 2008 The NetBSD Foundation, Inc.
 * All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
#ifndef lint
__RCSID("$NetBSD: main.c,v 1.10 2010/03/19 16:25:33 pooka Exp $");
#endif /* !lint */

#include <sys/module.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <err.h>

int	main(int, char **);
static void	usage(void) __dead;
static int	modstatcmp(const void *, const void *);

static const char *classes[] = {
	"any",
	"misc",
	"vfs",
	"driver",
	"exec",
	"secmodel",
};
const unsigned int class_max = __arraycount(classes);

static const char *sources[] = {
	"builtin",
	"boot",
	"filesys",
};
const unsigned int source_max = __arraycount(sources);

int
main(int argc, char **argv)
{
	struct iovec iov;
	modstat_t *ms;
	size_t len;
	const char *name;
	char sbuf[32];
	int ch;

	name = NULL;

	while ((ch = getopt(argc, argv, "n:")) != -1) {
		switch (ch) {
		case 'n':
			name = optarg;
			break;
		default:
			usage();
			/* NOTREACHED */
		}
	}

	argc -= optind;
	argv += optind;
	if (argc != 0)
		usage();

	for (len = 8192;;) {
		iov.iov_base = malloc(len);
		iov.iov_len = len;
		if (modctl(MODCTL_STAT, &iov)) {
			err(EXIT_FAILURE, "modctl(MODCTL_STAT)");
		}
		if (len >= iov.iov_len) {
			break;
		}
		free(iov.iov_base);
		len = iov.iov_len;
	}

	printf("%-16s %-10s %-10s %-5s %-8s %s\n",
	    "NAME", "CLASS", "SOURCE", "REFS", "SIZE", "REQUIRES");
	len = iov.iov_len / sizeof(modstat_t);
	qsort(iov.iov_base, len, sizeof(modstat_t), modstatcmp);
	for (ms = iov.iov_base; len != 0; ms++, len--) {
		const char *class;
		const char *source;

		if (name != NULL && strcmp(ms->ms_name, name) != 0) {
			continue;
		}
		if (ms->ms_required[0] == '\0') {
			ms->ms_required[0] = '-';
			ms->ms_required[1] = '\0';
		}
		if (ms->ms_size == 0) {
			sbuf[0] = '-';
			sbuf[1] = '\0';
		} else {
			snprintf(sbuf, sizeof(sbuf), "%u", ms->ms_size);
		}
		if (ms->ms_class <= class_max)
			class = classes[ms->ms_class];
		else
			class = "UNKNOWN";
		if (ms->ms_source < source_max)
			source = sources[ms->ms_source];
		else
			source = "UNKNOWN";

		printf("%-16s %-10s %-10s %-5d %-8s %s\n",
		    ms->ms_name, class, source, ms->ms_refcnt, sbuf,
		    ms->ms_required);
	}

	exit(EXIT_SUCCESS);
}

static void
usage(void)
{

	(void)fprintf(stderr, "Usage: %s [-n name]\n", getprogname());
	exit(EXIT_FAILURE);
}

static int
modstatcmp(const void *a, const void *b)
{
	const modstat_t *msa, *msb;

	msa = a;
	msb = b;

	return strcmp(msa->ms_name, msb->ms_name);
}