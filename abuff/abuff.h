
#ifndef BUFFER_H_
#define BUFFER_H_ 1

#include <stdio.h>
#include <stdlib.h>

struct abuff {
	int min;
	int max;
	int real;

	char *start;
	char *tail;
	char *end;
#if ABUFF_SHRINK
	int shrink;
#endif
};

#define abuff_is_full(buf)  (buf->max > 0 && buf->real >= buf->max)
#define abuff_left(buf)     (int)((buf)->end - (buf)->tail)
#define abuff_length(buf)   (int)((buf)->tail - (buf)->start)
#define abuff_string(buf)   ((buf)->start)
#define abuff_seal(buf) do {            \
	if ((buf)->end == (buf)->tail)  \
		*((buf)->tail - 1) = 0; \
	else                            \
		*(buf)->tail = 0;       \
} while(0)
#if ABUFF_SHRINK
#define abuff_restart(buf)  abuff_try_shrink(buf)
#else
#define abuff_restart(buf)  ((buf)->tail = (buf)->start)
#endif

struct abuff *abuff_new(size_t smin, size_t smax);
void abuff_destory(struct abuff *buf);
int abuff_append(struct abuff *buf, const char *str, int len);
int abuff_resize(struct abuff *buf, int expect);
int abuff_exponent_expand(struct abuff *buf);
void abuff_try_shrink(struct abuff *buf);

#endif

