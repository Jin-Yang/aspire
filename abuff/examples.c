#include "abuff.h"

#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>

#define READ_RETRY   -3
#define READ_EOF     -4
#define READ_FATAL   -5

#define read_wrap(b, l) do {                                                          \
        len = read(fd, b, l);                                                         \
        if (len < 0) {                                                                \
                if (errno == EAGAIN || errno == EINTR)                                \
                        return READ_RETRY;                                            \
                fprintf(stderr, "Read from #%d failed, %s\n", fd, strerror(errno));   \
                return READ_FATAL; /* maybe not fatal */                              \
        } else if (len == 0) {                                                        \
                return READ_EOF;                                                      \
        }                                                                             \
} while(0)

int buffer_read(int fd, struct abuff **buffer)
{
        struct abuff *buf;
        char dropbuf[128];
        int len = 0;

        if (buffer == NULL)
                return -1;
        buf = *buffer;

        if (buf == NULL) {
                buf = abuff_new(4, 4 * 16);
                if (buf == NULL) {
                        fprintf(stderr, "Create new buffer failed, out of memory\n");
                        return READ_FATAL;
                }
                len = buf->min;
                *buffer = buf;
        } else if (abuff_is_full(buf)) {
                read_wrap(dropbuf, sizeof(dropbuf));
                fprintf(stderr, "Read buffer is full now, drop %d bytes\n", len);
                return len;
        } else if (abuff_left(buf) < 1) {
                len = abuff_exponent_expand(buf);
                if (len < 0) {
                        fprintf(stderr, "Expand buffer failed, out of memory\n");
                        return READ_FATAL;
                }
                assert(len > 0);
        }

        fprintf(stdout, "Try to read %d bytes\n", len);
        read_wrap(buf->tail, len);
        buf->tail += len;
        fprintf(stdout, "        got %d bytes\n", len);

        return len;
}

int main(void)
{
        int rc, fd;
        struct abuff *buf = NULL;

        fd = open("test.txt", O_RDONLY);
        if (fd < 0) {
                fprintf(stderr, "Open file failed, %s\n", strerror(errno));
                return -1;
        }

        while (1) {
                rc = buffer_read(fd, &buf);
                if (rc == READ_EOF) {
                        break;
                } else if (rc < 0 && rc != READ_RETRY) {
                        fprintf(stderr, "Read failed, rc %d\n", rc);
                        return -1;
                }
        }

        abuff_seal(buf);

        fprintf(stdout, "Got %d types, '%s'\n", abuff_length(buf), abuff_string(buf));

        abuff_destory(buf);

        return 0;
}
