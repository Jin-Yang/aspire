#ifndef LIBMOCK_TESTING_H_
#define LIBMOCK_TESTING_H_

#include <stdlib.h>

/* This type should be large enough to hold any pointer or integer supported by the compiler. */
#if __WORDSIZE == 64
        #define maxint_t unsigned long int
        #define MAXINT_FORMAT   "%ld"
#else
        #define maxint_t unsigned long long int
        #define MAXINT_FORMAT   "%lld"
#endif

typedef int (*CMFixtureFunction)(void **state);

struct unit_test {
        const char *name;
        void (*test)(void **);
        int (*setup)(void **);
        int (*teardown)(void **);
};
struct source_location {
        const char *file;
        int line;
};

/* event that's called to check a parameter value. */
struct check_para_event {
        struct source_location loc;
        const char *parameter_name;
        int (*check_value)(const maxint_t, const maxint_t);
        maxint_t check_value_data;
};

int _mockx_run_group_tests(const char *name,
        const struct unit_test *const tests, const int num_tests,
        int (*setup)(void **), int (*teardown)(void **));

void _expect_string(const char *const func, const char *const file, const int line, const char *const para, const char *string, const int count);
void _expect_value(const char *const func, const char *const file, const int line, const char *const para, const maxint_t value, const int count);
void _expect_in_set(const char *const func, const char *const file, const int line, const char *const para, 
                const maxint_t values[], const int number_of_values, const int count);

void _will_return(const char *func, const char *file, const int line, const int value, const int count);

void _check_expected(const char *const func, const char *file, const int line, const char * const parameter_name, const maxint_t value);
maxint_t _mock(const char * const func, const char* const file, const int line);

void _assert_double_equal(const double a, const double b, const char *const file, const int line);
void _assert_int_equal(const maxint_t a, const maxint_t b, const char * const file, const int line);

#define mockx_unit_test(func, setup, teardown)        { #func, func, setup, teardown }
#define mockx_run_group_tests(tests, setup, teardown) \
        _mockx_run_group_tests(#tests, tests, sizeof(tests)/sizeof(tests)[0], setup, teardown)
#define expect_string(func, para, str)         _expect_string(#func, __FILE__, __LINE__, #para, (const char*)(str), 1)
#define expect_value(func, para, value)        _expect_value(#func, __FILE__, __LINE__, #para, (maxint_t)value, 1)
#define expect_in_set(func, para, array)       _expect_in_set(#func, __FILE__, __LINE__, #para, array, (int)(sizeof(array)/sizeof(array[0])), 1)
#define expect_in_range(func, para, min, max)  _expect_in_range(#func, __FILE__, __LINE__, #para, min, max, 1)

#define will_return(func, val)                 _will_return(#func, __FILE__, __LINE__, val, 1)
#define will_return_count(func, val, count)    _will_return(#func, __FILE__, __LINE__, val, count)
#define will_return_always(func, val)          _will_return(#func, __FILE__, __LINE__, val, -1)

#define check_expected(para)            _check_expected(__func__, __FILE__, __LINE__, #para, (const maxint_t)para)
#define mock()                          _mock(__func__, __FILE__, __LINE__)

#define assert_double_equal(a, b)       _assert_double_equal((double)(a), (double)(b), __FILE__, __LINE__)
#define assert_int_equal(a, b)          _assert_int_equal((maxint_t)(a), (maxint_t)(b), __FILE__, __LINE__)

#endif
