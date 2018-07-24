
#include "buffer.h"
#include <string.h>

static int abuffer_do_resize(struct abuffer *buf, int nsz)
{
        int osz; /* orignal size */
        char *tmp;

        _debug("resize >>>>>>, real=%d max=%d left=%d new=%d",
                        buf->real, buf->max, abuffer_left(buf), nsz);

        tmp = (char *)realloc(buf->start, nsz);
        if (tmp == NULL) /* use original */
                return -1;

        osz = abuffer_length(buf);
        buf->start = tmp;
        buf->end = buf->start + nsz;
        buf->tail = buf->start + osz;
        buf->real = nsz;

        _debug("resize <<<<<<, real=%d max=%d left=%d",
                        buf->real, buf->max, abuffer_left(buf));

        return abuffer_left(buf);
}

#define abuffer_check_size(buf)                                \
        do {                                                   \
                if (buf->max > 0 && buf->real >= buf->max) {   \
                        _error("reach top size %d", buf->max); \
                        return abuffer_left(buf);              \
                }                                              \
        } while(0)

int abuffer_exponent_expand(struct abuffer *buf)
{
        int nsz;

        abuffer_check_size(buf);

        nsz = buf->real * 2;
        if (nsz > buf->max)
                nsz = buf->max;

        return abuffer_do_resize(buf, nsz);
}

int abuffer_resize(struct abuffer *buf, int expect)
{
        int nsz; /* new size */

        abuffer_check_size(buf);

        for (nsz = buf->real; nsz < expect && nsz < buf->max; nsz *= 2);
        if (nsz > buf->max)
                nsz = buf->max;

        return abuffer_do_resize(buf, nsz);
}

int abuffer_append(struct abuffer *buf, const char *str, int len)
{
	int rc;
	int left;

	if (len <= 0 || str == NULL)
		return 0;

	left = abuffer_left(buf);
	if (left < len) {
		rc = abuffer_resize(buf, buf->real + (len - left));
		if (rc >= 0)
			len = rc > len ? len : rc;
		else
			return rc;
	}

	_debug("append %d bytes, real=%d max=%d left=%d",
			len, buf->real, buf->max, abuffer_left(buf));
	memcpy(buf->tail, str, len);
	buf->tail += len;

	return len;
}

void abuffer_destory(struct abuffer *buf)
{
	if (buf == NULL)
		return;

	if (buf->start)
		free(buf->start);

	free(buf);
}

struct abuffer *abuffer_new(size_t smin, size_t smax)
{
	struct abuffer *buf;

	if (smin == 0 || (smax != 0 && smax < smin))
		return NULL;

	buf = (struct abuffer *)calloc(1, sizeof(struct abuffer));
	if (buf == NULL)
		return NULL;

	buf->min = smin;
	buf->max = smax;
	buf->real = buf->min;

	buf->start = (char *)malloc(buf->real);
	buf->tail = buf->start;
	buf->end = buf->start + buf->real;

	return buf;
}

