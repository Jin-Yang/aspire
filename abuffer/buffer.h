
#ifndef BUFFER_H_
#define BUFFER_H_ 1

#include <stdio.h>
#include <stdlib.h>

struct abuffer {
	int min;
	int max;
	int real;

	char *start;
	char *tail;
	char *end;
};

#if defined __STDC_VERSION__ && __STDC_VERSION__ >= 199901L
	#define _debug(...) do { printf("debug: " __VA_ARGS__); putchar('\n'); } while(0);
	#define _warn(...)  do { printf("warn : " __VA_ARGS__); putchar('\n'); } while(0);
	#define _error(...) do { printf("error: " __VA_ARGS__); putchar('\n'); } while(0);
#elif defined __GNUC__
	#define _debug(fmt, args...)  do { printf("debug: " fmt, ## args); putchar('\n'); } while(0);
	#define _warn(fmt, args...)   do { printf("warn: "  fmt, ## args); putchar('\n'); } while(0);
	#define _error(fmt, args...)  do { printf("error: " fmt, ## args); putchar('\n'); } while(0);
#endif

#define abuffer_left(buf)     (int)((buf)->end - (buf)->tail)
#define abuffer_length(buf)   (int)((buf)->tail - (buf)->start)
#define abuffer_restart(buf)  (buf)->tail = (buf)->start;
#define abuffer_string(buf)   ((buf)->start)
#define abuffer_seal(buf)                       \
	do {                                    \
		if ((buf)->end == (buf)->tail)  \
			*((buf)->tail - 1) = 0; \
		else                            \
			*(buf)->tail = 0;       \
	} while(0)

struct abuffer *abuffer_new(size_t smin, size_t smax);
void abuffer_destory(struct abuffer *buf);
int abuffer_append(struct abuffer *buf, const char *str, int len);
int abuffer_resize(struct abuffer *buf, int expect);

#endif

