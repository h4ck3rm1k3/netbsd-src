/*      Id: crtend.c,v 1.2 2009/08/16 23:07:08 gmcgarry Exp 	*/	
/*      $NetBSD: crtend.c,v 1.1.1.3 2010/06/03 18:58:05 plunky Exp $	*/
/*-
 * Copyright (c) 2008 Gregory McGarry <g.mcgarry@ieee.org>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include "common.h"

asm(	"	.section .ctors\n"
#if defined(__x86_64__) || defined(__sparc64__)
	"	.quad 0\n"
#else
	"	.long 0\n"
#endif
);

asm(	"	.section .dtors\n"
#if defined(__x86_64__) || defined(__sparc64__)
	"	.quad 0\n"
#else
	"	.long 0\n"
#endif
);

IDENT("Id: crtend.c,v 1.2 2009/08/16 23:07:08 gmcgarry Exp ");	
IDENT("$NetBSD: crtend.c,v 1.1.1.3 2010/06/03 18:58:05 plunky Exp $");