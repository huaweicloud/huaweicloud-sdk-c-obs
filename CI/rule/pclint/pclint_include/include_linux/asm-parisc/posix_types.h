#ifndef __ARCH_PARISC_POSIX_TYPES_H
#define __ARCH_PARISC_POSIX_TYPES_H

/*
 * This file is generally used by user-level software, so you need to
 * be a little careful about namespace pollution etc.  Also, we cannot
 * assume GCC is being used.
 */
typedef unsigned long		__kernel_ino_t;
typedef unsigned short		__kernel_mode_t;
typedef unsigned short		__kernel_nlink_t;
typedef long			__kernel_off_t;
typedef int			__kernel_pid_t;
typedef unsigned short		__kernel_ipc_pid_t;
typedef unsigned int		__kernel_uid_t;
typedef unsigned int		__kernel_gid_t;
typedef int			__kernel_suseconds_t;
typedef long			__kernel_clock_t;
typedef int			__kernel_timer_t;
typedef int			__kernel_clockid_t;
typedef int			__kernel_daddr_t;
/* Note these change from narrow to wide kernels */
#ifdef CONFIG_64BIT
typedef unsigned long		__kernel_size_t;
typedef long			__kernel_ssize_t;
typedef long			__kernel_ptrdiff_t;
#else
typedef unsigned int		__kernel_size_t;
typedef int			__kernel_ssize_t;
typedef int			__kernel_ptrdiff_t;
#endif
typedef long			__kernel_time_t;
typedef char *			__kernel_caddr_t;

typedef unsigned short		__kernel_uid16_t;
typedef unsigned short		__kernel_gid16_t;
typedef unsigned int		__kernel_uid32_t;
typedef unsigned int		__kernel_gid32_t;

#ifdef __GNUC__
typedef long long		__kernel_loff_t;
typedef long long		__kernel_off64_t;
typedef unsigned long long	__kernel_ino64_t;
#endif

typedef unsigned int		__kernel_old_dev_t;

typedef struct {
	int	val[2];
} __kernel_fsid_t;

/* compatibility stuff */
typedef __kernel_uid_t __kernel_old_uid_t;
typedef __kernel_gid_t __kernel_old_gid_t;


#endif
