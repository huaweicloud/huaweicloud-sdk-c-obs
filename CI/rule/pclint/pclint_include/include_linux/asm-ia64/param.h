#ifndef _ASM_IA64_PARAM_H
#define _ASM_IA64_PARAM_H

/*
 * Fundamental kernel parameters.
 *
 * Based on <asm-i386/param.h>.
 *
 * Modified 1998, 1999, 2002-2003
 *	David Mosberger-Tang <davidm@hpl.hp.com>, Hewlett-Packard Co
 */

#define EXEC_PAGESIZE	65536

#ifndef NOGROUP
# define NOGROUP	(-1)
#endif

#define MAXHOSTNAMELEN	64	/* max length of hostname */

   /*
    * Technically, this is wrong, but some old apps still refer to it.  The proper way to
    * get the HZ value is via sysconf(_SC_CLK_TCK).
    */
# define HZ 1024

#endif /* _ASM_IA64_PARAM_H */
