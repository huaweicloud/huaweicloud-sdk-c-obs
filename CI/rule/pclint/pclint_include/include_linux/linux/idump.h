#ifdef CONFIG_IDUMP
/* Defining macros for idump */
#ifndef _LINUX_IDUMP_H
#define _LINUX_IDUMP_H

#define IDUMP_DEBUG 0

#if IDUMP_DEBUG
#define IDUMP_DBG(fmt, ...) printk(fmt, ##__VA_ARGS__)
#else
#define IDUMP_DBG(fmt, ...)
#endif

/* KSTK_ESP returns the stack pointer */
#define IDUMP_IS_STACK_GROWS_DOWN(task) \
((KSTK_ESP(task) < (task->mm->start_stack)) ? 1:0)
#define IDUMP_VMA_RW(vma) \
((vma->vm_flags & VM_READ) && (vma->vm_flags & VM_WRITE))
#define IDUMP_VMA_BIN_CODE(vma) \
(vma->vm_start >= mm->start_code && vma->vm_end >= mm->end_code)
#define IDUMP_VMA_BIN_DATA_RONLY(mm, vma)\
((vma->vm_start <= mm->start_data) && (vma->vm_end <= mm->end_data))
#define IDUMP_VMA_BIN_DATA_RW(mm, vma) \
((vma->vm_start >= mm->start_data) && (vma->vm_end >= mm->start_brk))
/* Binary Data can be read only or read-write */
#define IDUMP_VMA_BIN_DATA_ROW(vma) \
(((vma->vm_start <= mm->start_brk) && (vma->vm_end <= mm->start_brk)) \
&& (vma->vm_flags & (VM_READ | VM_WRITE)))
#define IDUMP_VMA_STACK_GROWS_DOWN(vma) \
((IDUMP_VMA_RW(vma)) && (vma->vm_flags & VM_GROWSDOWN))
#define IDUMP_VMA_STACK_GROWS_UP(vma) \
((IDUMP_VMA_RW(vma)) && (vma->vm_flags & VM_GROWSUP))
#define IDUMP_VMA_HEAP(vma) \
((vma->vm_start >= mm->start_brk) && (vma->vm_start <= mm->brk))
#define IDUMP_VMA_CODE(vma) \
((vma->vm_flags & VM_READ) && (vma->vm_flags & VM_EXEC))
#define IDUMP_VMA_DATA(vma) \
(IDUMP_VMA_BIN_DATA_RONLY(mm, vma) || IDUMP_VMA_BIN_DATA_RW(mm, vma))
#define IDUMP_FILE_CHECK(vma) \
(vma->vm_file?1:0)
#define IDUMP_VMA_ANON_SHARED(vma) \
((vma->vm_flags & VM_SHARED) && (IDUMP_FILE_CHECK(vma)) \
&& (vma->vm_file->f_path.dentry->d_inode->i_nlink == 0))
#define IDUMP_VMA_ANON_PRIVATE(vma) (vma->anon_vma)
#define IDUMP_ALIGN(esp)\
(esp + ((PAGE_SIZE) - (esp % PAGE_SIZE)))

#define IDUMP_USED_STACK  "used"
/* Boolean literals */
enum {
	IDUMP_FALSE = 0,
	IDUMP_TRUE = 1,
	IDUMP_EXTRA_SIZE = 7,
	IDUMP_FULL_STACK = 0
};
#endif
#endif
