/*	$NetBSD: kvatoname.c,v 1.1.1.3 2010/04/17 20:45:49 darrenr Exp $	*/

/*
 * Copyright (C) 2002 by Darren Reed.
 *
 * See the IPFILTER.LICENCE file for details on licencing.
 *
 * Id: kvatoname.c,v 1.1.4.2 2009/12/27 06:58:06 darrenr Exp
 */

#include "ipf.h"

#include <fcntl.h>
#include <sys/ioctl.h>

char *kvatoname(func, iocfunc)
ipfunc_t func;
ioctlfunc_t iocfunc;
{
	static char funcname[40];
	ipfunc_resolve_t res;
	int fd;

	res.ipfu_addr = func;
	res.ipfu_name[0] = '\0';
	fd = -1;

	if ((opts & OPT_DONOTHING) == 0) {
		fd = open(IPL_NAME, O_RDONLY);
		if (fd == -1)
			return NULL;
	}
	(void) (*iocfunc)(fd, SIOCFUNCL, &res);
	if (fd >= 0)
		close(fd);
	strncpy(funcname, res.ipfu_name, sizeof(funcname));
	funcname[sizeof(funcname) - 1] = '\0';
	return funcname;
}