
#include "abuff.h"
#include <string.h>

#if defined __STDC_VERSION__ && __STDC_VERSION__ >= 199901L
	#define _debug(...) do { printf("debug: " __VA_ARGS__); putchar('\n'); } while(0);
#elif defined __GNUC__
	#define _debug(fmt, args...)  do { printf("debug: " fmt, ## args); putchar('\n'); } while(0);
#endif

static int abuff_do_resize(struct abuff *buf, int nsz)
{
        int osz; /* orignal size */
        char *tmp;

        _debug("resize >>>>>>, real=%d max=%d left=%d new=%d",
                        buf->real, buf->max, abuff_left(buf), nsz);

        tmp = (char *)realloc(buf->start, nsz);
        if (tmp == NULL) /* use original */
                return -1;

        osz = abuff_length(buf);
        buf->start = tmp;
        buf->end = buf->start + nsz;
        buf->tail = buf->start + osz;
        buf->real = nsz;

        _debug("resize <<<<<<, real=%d max=%d left=%d",
                        buf->real, buf->max, abuff_left(buf));

        return abuff_left(buf);
}

#define abuff_check_size(buf) do {                     \
	if (buf->max > 0 && buf->real >= buf->max) {   \
		_debug("reach top size %d", buf->max); \
		return abuff_left(buf);                \
	}                                              \
} while(0)

int abuff_exponent_expand(struct abuff *buf)
{
        int nsz;

        abuff_check_size(buf);
        nsz = buf->real * 2;
        if (nsz > buf->max)
                nsz = buf->max;

        return abuff_do_resize(buf, nsz);
}

int abuff_resize(struct abuff *buf, int expect)
{
        int nsz; /* new size */

        abuff_check_size(buf);

        for (nsz = buf->real; nsz < expect && nsz < buf->max; nsz *= 2);
        if (nsz > buf->max)
                nsz = buf->max;

        return abuff_do_resize(buf, nsz);
}

#if ABUFF_SHRINK
void abuff_try_shrink(struct abuff *buf)
{
	int osz, nsz;
	char *tmp;


	osz = abuff_length(buf);
	nsz = buf->real / 2;
	buf->tail = buf->start;
	if (osz > nsz) {
		buf->shrink = 0;
		return;
	}

	if (++buf->shrink >= ABUFF_SHRINK) {
		_debug("shrink >>>>>>, real=%d max=%d length=%d nsz=%d",
			buf->real, buf->max, osz, nsz);
		tmp = (char *)realloc(buf->start, nsz);
		if (tmp == NULL) /* use original */
			return;
		buf->start = tmp;
		buf->end = buf->start + nsz;
		buf->tail = tmp;
		buf->real = nsz;
		buf->shrink = 0;
	}
}
#endif

int abuff_append(struct abuff *buf, const char *str, int len)
{
	int rc;
	int left;

	if (len <= 0 || str == NULL)
		return 0;

	left = abuff_left(buf);
	if (left < len) {
		rc = abuff_resize(buf, buf->real + (len - left));
		if (rc >= 0)
			len = rc > len ? len : rc;
		else
			return rc;
	}
	memcpy(buf->tail, str, len);
	buf->tail += len;
	_debug("append %d bytes, real=%d max=%d left=%d",
			len, buf->real, buf->max, abuff_left(buf));

	return len;
}

void abuff_destory(struct abuff *buf)
{
	if (buf == NULL)
		return;

	if (buf->start)
		free(buf->start);

	free(buf);
}

struct abuff *abuff_new(size_t smin, size_t smax)
{
	struct abuff *buf;

	if (smin == 0 || (smax != 0 && smax < smin))
		return NULL;

	buf = (struct abuff *)malloc(sizeof(struct abuff));
	if (buf == NULL)
		return NULL;

	buf->min = smin; buf->max = smax;
	buf->real = buf->min;

	buf->start = (char *)malloc(buf->real);
	buf->tail = buf->start;
	buf->end = buf->start + buf->real;

#if ABUFF_SHRINK
	buf->shrink = 0;
#endif

	return buf;
}

