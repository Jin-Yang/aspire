
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
};

#if defined __STDC_VERSION__ && __STDC_VERSION__ >= 199901L
	#define _debug(...) do { printf("debug: " __VA_ARGS__ "\n"); } while(0);
	#define _info(...)  do { printf("info : " __VA_ARGS__ "\n"); } while(0);
	#define _warn(...)  do { printf("warn : " __VA_ARGS__ "\n"); } while(0);
	#define _error(...) do { printf("error: " __VA_ARGS__ "\n"); } while(0);
#elif defined __GNUC__
	#define _debug(fmt, args...)  do { printf("debug: " fmt, ## args "\n"); } while(0);
	#define _info(fmt, args...)   do { printf("info : " fmt, ## args "\n"); } while(0);
	#define _warn(fmt, args...)   do { printf("warn : " fmt, ## args "\n"); } while(0);
	#define _error(fmt, args...)  do { printf("error: " fmt, ## args "\n"); } while(0);
#endif

#define abuff_is_full(buf)  (buf->max > 0 && buf->real >= buf->max)
#define abuff_left(buf)     (int)((buf)->end - (buf)->tail)
#define abuff_length(buf)   (int)((buf)->tail - (buf)->start)
#define abuff_restart(buf)  (buf)->tail = (buf)->start;
#define abuff_string(buf)   ((buf)->start)
#define abuff_seal(buf)                       \
	do {                                    \
		if ((buf)->end == (buf)->tail)  \
			*((buf)->tail - 1) = 0; \
		else                            \
			*(buf)->tail = 0;       \
	} while(0)

struct abuff *abuff_new(size_t smin, size_t smax);
void abuff_destory(struct abuff *buf);
int abuff_append(struct abuff *buf, const char *str, int len);
int abuff_resize(struct abuff *buf, int expect);
int abuff_exponent_expand(struct abuff *buf);

#endif

