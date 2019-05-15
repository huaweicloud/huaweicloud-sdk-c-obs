#ifndef __LINUX_CPUTIME_H
#define __LINUX_CPUTIME_H

#include <asm/cputime.h>

#ifndef cputime_to_nsecs
# define cputime_to_nsecs(__ct)	\
	(cputime_to_usecs(__ct) * NSEC_PER_USEC)
#endif

#ifndef nsecs_to_cputime
# define nsecs_to_cputime(__nsecs)	\
	usecs_to_cputime((__nsecs) / NSEC_PER_USEC)
#endif

#ifdef CONFIG_USE_SCHED_IDLE_TIME
extern int use_sched_idle_time;
extern int sched_idle_time_adjust(int cpu, u64 *utime, u64 *stime);
extern unsigned long long sched_get_idle_time(int cpu);
extern u64 get_idle_time(int cpu);
#endif

#endif /* __LINUX_CPUTIME_H */
