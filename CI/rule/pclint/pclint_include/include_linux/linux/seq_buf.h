#ifndef _LINUX_SEQ_BUF_H
#define _LINUX_SEQ_BUF_H

#include <linux/fs.h>

/*
 * Trace sequences are used to allow a function to call several other functions
 * to create a string of data to use.
 */

/**
 * seq_buf - seq buffer structure
 * @buffer:	pointer to the buffer
 * @size:	size of the buffer
 * @len:	the amount of data inside the buffer
 * @readpos:	The next position to read in the buffer.
 */
struct seq_buf {
	char			*buffer;
	size_t			size;
	size_t			len;
	loff_t			readpos;
};

static inline void seq_buf_clear(struct seq_buf *s)
{
	s->len = 0;
	s->readpos = 0;
}

static inline void
seq_buf_init(struct seq_buf *s, unsigned char *buf, unsigned int size)
{
	s->buffer = buf;
	s->size = size;
	seq_buf_clear(s);
}

static inline void
seq_buf_set_overflow(struct seq_buf *s)
{
	s->len = s->size + 1;
}

/* How much buffer was written? */
static inline unsigned int seq_buf_used(struct seq_buf *s)
{
	return min(s->len, s->size);
}

extern __printf(2, 0)
int seq_buf_vprintf(struct seq_buf *s, const char *fmt, va_list args);
#endif /* _LINUX_SEQ_BUF_H */
