#include "../load.h"
#include "../libs/libmock/testing.h"

#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#define unused __attribute__((__unused__))

#define MOCK_MIN_FD  4096

static void test_string_split(unused void **state) {
}

int __real_open(const char *path, int flags, int mode);
int __wrap_open(const char *path, int flags, int mode)
{
        if (strlen(path) > 5 && !strcmp(path + strlen(path) - 5, ".gcda"))
                return __real_open(path, flags, mode);

        check_expected(path);
        check_expected(flags);

        return mock();
}

ssize_t __real_read(int fildes, void *buf, size_t nbyte);
ssize_t __wrap_read(int fildes, void *buf, size_t nbyte)
{
        size_t len = 0;
        const char *data;

        if (fildes == MOCK_MIN_FD) {
                data = "0.09 0.12 0.17 1/1353 2566";
                len = sizeof("0.09 0.12 0.17 1/1353 2566") - 1;
        }

        if (nbyte < len)
                len = nbyte;
        if (len > 0)
                memcpy(buf, data, len);

        return len;
}

int __real_close(int fd);
int __wrap_close(int fd)
{
        if (fd >= MOCK_MIN_FD)
                return 0;
        return __real_close(fd);
}

static void test_read_loadavg(unused void **state)
{
        int rc;
        double load[3];
        maxint_t flags[] = {O_RDONLY};

        expect_string(__wrap_open, path, "/proc/loadavg");
        //expect_value(__wrap_open, flags, O_RDONLY);
        expect_in_set(__wrap_open, flags, flags);
        will_return(__wrap_open, MOCK_MIN_FD);
        //will_return_count(__wrap_open, MOCK_MIN_FD, 10);

        rc = read_loadavg(load);
        assert_int_equal(0, rc);

        assert_double_equal(0.09, load[0]);
        assert_double_equal(0.12, load[1]);
        assert_double_equal(0.17, load[2]);
}

int main(void)
{
        const struct unit_test tests[] = {
                mockx_unit_test(test_string_split, NULL, NULL),
                mockx_unit_test(test_read_loadavg, NULL, NULL),
        };

        return mockx_run_group_tests(tests, NULL, NULL);
}
