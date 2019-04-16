#include "testing.h"

#include <math.h>
#include <stdio.h>
#include <assert.h>
#include <stdarg.h>
#include <string.h>
#include <setjmp.h>
#include <sys/time.h>

#define log_info(...)  do { printf(" info: " __VA_ARGS__); putchar('\n'); } while(0);
#define log_error(...) do { printf("error: " __VA_ARGS__); putchar('\n'); } while(0);

#define SRC_LOC_FORMAT "%s:%u"

#define libc_malloc       malloc
#define libc_realloc      realloc
#define libc_free         free

enum test_status {
        CM_TEST_NOT_STARTED,
        CM_TEST_PASSED,
        CM_TEST_FAILED,
        CM_TEST_ERROR,
        CM_TEST_SKIPPED,
};

enum printf_type {
        PRINTF_TEST_START,
        PRINTF_TEST_SUCCESS,
        PRINTF_TEST_FAILURE,
        PRINTF_TEST_ERROR,
        PRINTF_TEST_SKIPPED,
};

/* doubly linked list node. */
struct lnode {
        const void *value;
        int refcount;
        struct lnode *next, *prev;
};

struct symbol_value {
        struct source_location location;
        maxint_t value;
};

struct symbol_map_value {
        const char *symbol_name;
        struct lnode symbol_values_list_head;
};

struct test_state {
        const struct unit_test *test; /* Point to array element in the tests we get passed */
        int status;                   /* PASSED, FAILED, ABORT ... */
        void *state;                  /* State associated with the test */
        double runtime;               /* Time calculations */
        const char *error_message;    /* The error messages by the test */

#if 0
        const ListNode *check_point; /* Check point of the test if there's a setup function. */
#endif
};

struct check_integer_set {
        struct check_para_event event;
        const maxint_t *set;
        int nums;
};

struct check_integer_range {
        struct check_para_event event;
        maxint_t min, max;
};

static int error_message_enabled = 1;
static char *global_error_message;

static int global_skip_test;
static int global_running_test;
static jmp_buf global_run_test_env;

/* keeps a map of the values that functions will have to return to provide */
static struct lnode global_function_result_map_head;
static struct source_location global_last_mock_value_location;

static struct lnode global_function_parameter_map_head;
/* location of last parameter value checked was declared. */
static struct source_location global_last_parameter_location;

static int list_find(struct lnode *const head, const void *value,
                int (*cmp)(const void *, const void *), struct lnode **ret)
{
        struct lnode *curr;
        //assert_non_null(head);

        for (curr = head->next; curr != head; curr = curr->next) {
                if (cmp(curr->value, value)) {
                        *ret = curr;
                        return 1;
                }
        }
        return 0;
}

/* Initialize a list node. */
void list_initialize(struct lnode *const node)
{
        node->value = NULL;
        node->next = node;
        node->prev = node;
        node->refcount = 1;
}

/* determine whether a list is empty. */
static int list_empty(const struct lnode * const head)
{
        //assert_non_null(head);
        return head->next == head;
}
/* returns the first node of a list */
static int list_first(struct lnode *const head, struct lnode **output)
{
        //assert_non_null(head);
        if (list_empty(head))
                return 0;
        *output = head->next;
        return 1;
}

/* add new_node to the end of the list. */
static void list_add(struct lnode *const head, struct lnode *new_node)
{
#if 0
        assert_non_null(head);
        assert_non_null(new_node);
#endif
        new_node->next = head;
        new_node->prev = head->prev;
        head->prev->next = new_node;
        head->prev = new_node;
}

/* remove a node from a list. */
static struct lnode *list_remove(struct lnode *const node,
                void (*cleanup)(const void *, void *), void *const data)
{
        //assert_non_null(node);
        node->prev->next = node->next;
        node->next->prev = node->prev;
        if (cleanup)
                cleanup(node->value, data);
        return node;
}

/* remove a list node from a list and free the node. */
static void list_remove_free(struct lnode *const node,
                void (*cleanup)(const void *, void *), void *const data)
{
        //assert_non_null(node);
        free(list_remove(node, cleanup, data));
}

static struct lnode *list_free(struct lnode *const head,
                void (*cleanup)(const void *, void *), void *const data)
{
        //assert_non_null(head);
        while (!list_empty(head))
                list_remove_free(head->next, cleanup, data);
        return head;
}


static struct lnode *list_add_value(struct lnode *const head, const void *value, const int refcount)
{
        struct lnode * const node = (struct lnode *)malloc(sizeof(*node));
#if 0
        assert_non_null(head);
        assert_non_null(value);
#endif
        node->value = value;
        node->refcount = refcount;
        list_add(head, node);

        return node;
}
static void initialize_source_location(struct source_location *const loc)
{
        //assert_non_null(location);
        loc->file = NULL;
        loc->line = 0;
}

/* Determine whether a source location is currently set. */
static int source_location_is_set(const struct source_location *const loc)
{
        //assert_non_null(location);
        return loc->file && loc->line;
}


/* Deallocate a value referenced by a list. */
static void free_value(const void *value, void *cleanup_value_data)
{
        (void) cleanup_value_data;
        //assert_non_null(value);
        free((void*)value);
}

/* releases memory associated to a symbol_map_value. */
static void free_symbol_map_value(const void *value, void *cleanup_value_data)
{
        struct symbol_map_value *const sym = (struct symbol_map_value *)value;
        const maxint_t children = (maxint_t)cleanup_value_data;
        //assert_non_null(value);

        list_free(&sym->symbol_values_list_head,
                        children ? free_symbol_map_value : free_value, (void *)(children - 1));
        free(sym);
}

static void teardown_testing(void)
{
        list_free(&global_function_result_map_head, free_symbol_map_value, (void*)0);
        initialize_source_location(&global_last_mock_value_location);

        list_free(&global_function_parameter_map_head, free_symbol_map_value, (void*)1);
        initialize_source_location(&global_last_parameter_location);
}

static int symbol_names_match(const void *map_value, const void *symbol) {
        return !strcmp(((struct symbol_map_value*)map_value)->symbol_name, (const char*)symbol);
}

static void add_symbol_value(struct lnode *const head,
                             const char *const symbol_names[],
                             const int number_of_symbol_names,
                             const void *value, const int refcount)
{
        const char* symbol_name;
        struct lnode *target_node;
        struct symbol_map_value *sym, *target_map_value;

#if 0
        assert_non_null(symbol_map_head);
        assert_non_null(symbol_names);
        assert_true(number_of_symbol_names);
#endif

        symbol_name = symbol_names[0];
        if (!list_find(head, symbol_name, symbol_names_match, &target_node)) {
                sym = (struct symbol_map_value *)malloc(sizeof(*sym)); // TODO

                sym->symbol_name = symbol_name;
                list_initialize(&sym->symbol_values_list_head);
                target_node = list_add_value(head, sym, 1);
        }

        target_map_value = (struct symbol_map_value *)target_node->value;
        if (number_of_symbol_names == 1) {
                list_add_value(&target_map_value->symbol_values_list_head, value, refcount);
        } else {
                add_symbol_value(&target_map_value->symbol_values_list_head,
                        &symbol_names[1], number_of_symbol_names - 1, value, refcount);
        }
}

/* exit the currently executing test. */
static void exit_test(const int quit)
{
        const char *abort_test = getenv("MOCKX_TEST_ABORT");

        if (abort_test != NULL && abort_test[0] == '1') {
                log_error("%s", global_error_message);
                abort();
        } else if (global_running_test) {
                longjmp(global_run_test_env, 1);
        } else if (quit) {
                exit(-1);
        }
}
/* try best to format a error message */
static void _do_format_error_msg(const char* const format, va_list args)
{
        va_list ap;
        int msg_len = 0, len;
        char buffer[1024], *tmp;

        len = vsnprintf(buffer, sizeof(buffer), format, args);
        if (len < 0)
                return;

        if (global_error_message == NULL) {
                /* CREATE MESSAGE */
                global_error_message = (char *)libc_malloc(len + 1);
                if (global_error_message == NULL)
                        return;
        } else {
                /* APPEND MESSAGE */
                msg_len = strlen(global_error_message);
                tmp = (char *)libc_realloc(global_error_message, msg_len + len + 1);
                if (tmp == NULL)
                        return;
                global_error_message = tmp;
        }

        if (len < (int)sizeof(buffer)) {
                /* use len + 1 to also copy '\0' */
                memcpy(global_error_message + msg_len, buffer, len + 1);
        } else {
                va_copy(ap, args);
                vsnprintf(global_error_message + msg_len, len, format, ap);
                va_end(ap);
        }
}
void format_error_msg(const char * const format, ...)
{
        va_list args;
        va_start(args, format);
        if (error_message_enabled)
                _do_format_error_msg(format, args);
        else
                vfprintf(stderr, format, args);
        va_end(args);
}


void _fail(const char * const file, const int line)
{
        format_error_msg(SRC_LOC_FORMAT " Failure!", file, line);
        exit_test(1);
}

void _expect_check(const char *const func, const char *const file, const int line,
                const char *const para,
                int (*check_func)(const maxint_t, const maxint_t),
                const maxint_t check_data,
                struct check_para_event *const event, const int refcount)
{
        struct check_para_event *const check =
                event ? event : (struct check_para_event *)malloc(sizeof(*check));
        const char *symbols[] = {func, para};

        check->parameter_name = para;
        check->check_value = check_func;
        check->check_value_data = check_data;
        check->loc.file = file;
        check->loc.line = line;

        add_symbol_value(&global_function_parameter_map_head, symbols, 2, check, refcount);
}

/* CheckParameterValue callback to check whether a parameter equals a string. */
static int check_string(const maxint_t value, const maxint_t expect)
{
        if (strcmp((char *)value, (char *)expect) == 0)
                return 1;

        format_error_msg("'%s' != '%s'", (char *)value, (char *)expect);
        return 0;
}

/* add an event to check whether a parameter is equal to a string. */
void _expect_string(const char *const func, const char *const file, const int line,
        const char *const para, const char *string, const int count)
{
        _expect_check(func, file, line, para, check_string, (maxint_t)string, NULL, count);
}

static int values_equal_display_error(const maxint_t left, const maxint_t right)
{
        if (left != right) {
                format_error_msg(MAXINT_FORMAT " != " MAXINT_FORMAT, left, right);
                return 0;
        }
        return 1;
}
/* CheckParameterValue callback to check whether a value is equal to an expected value. */
static int check_value(const maxint_t value, const maxint_t expect)
{
        return values_equal_display_error(value, expect);
}

/* add an event to check a parameter equals an expected value. */
void _expect_value(const char *const func, const char *const file, const int line,
                const char *const para, const maxint_t value, const int count)
{
        _expect_check(func, file, line, para, check_value, value, NULL, count);
}

static void expect_set(const char *const func, const char *const file, const int line, const char *const para,
                const maxint_t values[], const int numval,
                int (*check_func)(const maxint_t, const maxint_t), const int count)
{
        struct check_integer_set *const check_set =
                        (struct check_integer_set *)malloc(sizeof(*check_set) + (sizeof(values[0]) * numval));
        maxint_t *const set = (maxint_t *)(check_set + 1);

        //assert_non_null(values);
        //assert_true(number_of_values);
        memcpy(set, values, numval * sizeof(values[0]));
        check_set->set = set;
        check_set->nums = numval;

        _expect_check(func, file, line, para, check_func, (const maxint_t)check_set, &check_set->event, count);
}

static int value_in_set_display_error(const maxint_t value, const struct check_integer_set *const set, const int invert)
{
        int i;
        int succeeded = invert;
        //assert_non_null(check_integer_set);

        for (i = 0; i < set->nums; i++) {
                if (set->set[i] == value) {
                        /* If invert = 0 and item is found, succeeded = 1. */
                        /* If invert = 1 and item is found, succeeded = 0. */
                        succeeded = !succeeded;
                        break;
                }
        }
        if (succeeded)
                return 1;

        format_error_msg(MAXINT_FORMAT " is %sin the set (", value, invert ? "" : "not ");
        for (i = 0; i < set->nums; i++)
                format_error_msg(MAXINT_FORMAT ",", set->set[i]);
        format_error_msg(")");

        return 0;
}

/* CheckParameterValue callback to check whether a value is within a set. */
static int check_in_set(const maxint_t value, const maxint_t expect)
{
        return value_in_set_display_error(value, (const struct check_integer_set *)expect, 0);
}
/* Add an event to check whether a value is in a set. */
void _expect_in_set(const char *const func, const char *const file, const int line, const char *const para,
                const maxint_t values[], const int number_of_values, const int count)
{
        expect_set(func, file, line, para, values, number_of_values, check_in_set, count);
}

static void expect_range(const char *const func, const char *const para, const char *const file, const int line,
                const maxint_t min, const maxint_t max,
                int (*check_func)(const maxint_t, const maxint_t), const int count)
{
        struct check_integer_range *const check_range =
                (struct check_integer_range *)malloc(sizeof(*check_range));
        check_range->min = min;
        check_range->max = max;
        _expect_check(func, file, line, para, check_func, (const maxint_t)check_range, &check_range->event, count);
}

/* CheckParameterValue callback to check whether a value is within a range. */
static int check_in_range(const maxint_t value, const maxint_t check_value_data)
{
#if 0
    CheckIntegerRange * const check_integer_range =
        cast_largest_integral_type_to_pointer(CheckIntegerRange*,
                                              check_value_data);
    assert_non_null(check_integer_range);
    return integer_in_range_display_error(value, check_integer_range->minimum,
                                          check_integer_range->maximum);
#endif
}


/* Add an event to determine whether a parameter is within a range. */
void _expect_in_range(const char *const func, const char *const file, const int line, const char *const para,
                const maxint_t min, const maxint_t max, const int count)
{
        expect_range(func, para, file, line, min, max, check_in_range, count);
}


void _will_return(const char *func, const char *file, const int line, const int value, const int count)
{
        struct symbol_value *ret = (struct symbol_value *)malloc(sizeof(*ret));

        //assert_true(count > 0 || count == -1);
        ret->value = value;
        ret->location.file = file;
        ret->location.line = line;
        add_symbol_value(&global_function_result_map_head, &func, 1, ret, count);
}

static int get_symbol_value(struct lnode  *const head, const char *const symbol_names[],
                const size_t number_of_symbol_names, void **output)
{
        int rc;
        const char *symbol_name;
        struct symbol_map_value *map_value;
        struct lnode *target_node, *child_list, *value_node;

#if 0
        assert_non_null(head);
        assert_non_null(symbol_names);
        assert_true(number_of_symbol_names);
        assert_non_null(output);
#endif
        symbol_name = symbol_names[0];

        if (list_find(head, symbol_name, symbol_names_match, &target_node)) {
#if 0
                assert_non_null(target_node)
                assert_non_null(target_node->value);
#endif
                rc = 0;
                map_value = (struct symbol_map_value *)target_node->value;
                child_list = &map_value->symbol_values_list_head;

                if (number_of_symbol_names == 1) {
                        value_node = NULL;
                        rc = list_first(child_list, &value_node);
                        //assert_true(return_value);
                        *output = (void *)value_node->value;
                        rc = value_node->refcount;
                        //log_debug("get symbol %s, count %d.", symbol_name, value_node->refcount);
                        if (--value_node->refcount == 0) {
                                list_remove_free(value_node, NULL, NULL);
                        }
                } else {
                        rc = get_symbol_value(child_list, &symbol_names[1],
                                        number_of_symbol_names - 1, output);
                }
                if (list_empty(child_list))
                        list_remove_free(target_node, free_symbol_map_value, (void*)0);

                return rc;
        } else {
                log_error("No entries for symbol %s.", symbol_name);
        }

        return 0;
}

void _check_expected(const char *const func, const char *file, const int line,
                const char * const para, const maxint_t value)
{
        int rc;
        void *result;
        const char *symbols[] = {func, para};
        struct check_para_event *check;
        int check_succeeded;

        rc = get_symbol_value(&global_function_parameter_map_head, symbols, 2, &result);
        if (rc) {
                check = (struct check_para_event *)result;
                global_last_parameter_location = check->loc;
                check_succeeded = check->check_value(value, check->check_value_data);
                if (rc == 1)
                        free(check);
                if (!check_succeeded) {
                        log_error(SRC_LOC_FORMAT " Check of <%s(%s)> failed, @"
                                        SRC_LOC_FORMAT, file, line,
                                        func, para,
                                        global_last_parameter_location.file,
                                        global_last_parameter_location.line);
                        _fail(file, line);
                }
        } else {
                log_error(SRC_LOC_FORMAT " Could not get value to check <%s(%s)>.",
                                file, line, func, para);
                if (source_location_is_set(&global_last_parameter_location)) {
                        log_error(SRC_LOC_FORMAT " Previously declared parameter value was declared.",
                                global_last_parameter_location.file,
                                global_last_parameter_location.line);
                } else {
				                        log_error("There were no previously declared parameter values "
                                "for this test.");
                }
                exit_test(1);
        }
}

maxint_t _mock(const char * const func, const char* const file, const int line)
{
        void *result;
        struct symbol_value *symbol;
        maxint_t value;
        int rc;

        rc = get_symbol_value(&global_function_result_map_head, &func, 1, &result);
        if (rc) {
                symbol = (struct symbol_value *)result;
                value = symbol->value;
                global_last_mock_value_location = symbol->location;
                if (rc == 1)
                        free(symbol);
                return value;
        } else {
                log_error(SRC_LOC_FORMAT " Could not get value to mock function %s.",
                                file, line, func);
                if (source_location_is_set(&global_last_mock_value_location)) {
                        log_error(SRC_LOC_FORMAT " Previously returned mock value was "
                                "declared here.",
                                global_last_mock_value_location.file,
                                global_last_mock_value_location.line);
                } else {
                        log_error("There were no previously returned mock values for "
                           "this test.");
                }
                exit_test(1);
        }

        return 0;
}

void _assert_double_equal(const double a, const double b, const char *const file, const int line)
{
        if (fabs(a - b) > 0.00001)
                _fail(file, line);
}

void _assert_int_equal(const maxint_t a, const maxint_t b, const char *const file, const int line)
{
        if (values_equal_display_error(a, b) == 0)
                _fail(file, line);
}

static void printf_info(int type, const char *name, const char *error)
{
        switch (type) {
        case PRINTF_TEST_START:
                log_info("[ RUN      ] %s", name);
                break;
        case PRINTF_TEST_SUCCESS:
                log_info("[       OK ] %s", name);
                break;
        case PRINTF_TEST_FAILURE:
                log_info("[   FAILED ] %s (%s)", name, error ? error : "empty");
                break;
        case PRINTF_TEST_SKIPPED:
                log_info("[  SKIPPED ] %s", name);
                break;
        case PRINTF_TEST_ERROR:
		                log_info("[    ERROR ] %s", name);
                break;
        default:
                log_info("[  UNKNOWN ] %s", name);
        }
}

static void remove_always_return_values(struct lnode * const head, const size_t number_of_symbol_names)
{
        struct symbol_map_value *value;
        struct lnode *curr, *next, *child, *node;
        //assert_non_null(map_head);
        //assert_true(number_of_symbol_names);

        for (curr = head->next; curr != head; curr = next) {
                value = (struct symbol_map_value *)curr->value;
                next = curr->next;
                //assert_non_null(value);

                child = &value->symbol_values_list_head;
                if (!list_empty(child)) {
                        if (number_of_symbol_names == 1) {
                                node = child->next;
                                /* If this item has been returned more than once, free it. */
                                if (node->refcount < -1)
                                        list_remove_free(node, free_value, NULL);
                        } else {
                                remove_always_return_values(child, number_of_symbol_names - 1);
                        }
                }
                if (list_empty(child))
                        list_remove_free(curr, free_value, NULL);
        }
}

static int check_for_leftover_values(const struct lnode *const map_head,
                const char * const error_message, const size_t number_of_symbol_names)
{
        struct symbol_map_value *value;
        const struct lnode *curr, *child, *node;
        struct source_location *location;
        int symbols_with_leftover_values = 0;

        //assert_non_null(map_head);
        //assert_true(number_of_symbol_names);

        for (curr = map_head->next; curr != map_head; curr = curr->next) {
                value = (struct symbol_map_value *)curr->value;
                //assert_non_null(value);
                //log_debug("current leftover %s %s.", value->symbol_name, error_message);

                child = &value->symbol_values_list_head;
                if (!list_empty(child)) {
                        if (number_of_symbol_names == 1) {
                                format_error_msg(error_message, value->symbol_name);
                                for (node = child->next; node != child; node = node->next) {
                                        location = (struct source_location *)node->value;
                                        format_error_msg(SRC_LOC_FORMAT ": remaining item was declared here.",
                                                        location->file, location->line);
                                }
                        } else {
                                format_error_msg("%s.", value->symbol_name);
                                check_for_leftover_values(child, error_message, number_of_symbol_names - 1);
                        }
                        symbols_with_leftover_values ++;
                }
        }
		        return symbols_with_leftover_values;
}

static void fail_if_leftover_values(void)
{
        int error_occurred = 0;

        remove_always_return_values(&global_function_result_map_head, 1);
        if (check_for_leftover_values(&global_function_result_map_head,
                                "%s() has remaining non-returned values.", 1))
                error_occurred = 1;

        remove_always_return_values(&global_function_parameter_map_head, 2);
        if (check_for_leftover_values(&global_function_parameter_map_head,
                                "%s parameter still has values that haven't been checked.\n", 2))
                error_occurred = 1;

        if (error_occurred)
                exit_test(1);
}

/* create function results and expected parameter lists. */
void initialize_testing(void)
{
        list_initialize(&global_function_result_map_head);
        initialize_source_location(&global_last_mock_value_location);

        list_initialize(&global_function_parameter_map_head);
        initialize_source_location(&global_last_parameter_location);
}

static int mockx_run_one_test_or_fixture(const struct unit_test *test, void **state)
{
        int rc = -1;

        /* init the test structure */
        initialize_testing();

        global_running_test = 1;

        if (setjmp(global_run_test_env) == 0) {
                if (test->test != NULL) {
                        test->test(state);
                        //fail_if_blocks_allocated(check_point, function_name);
                        rc = 0;
                } else if (test->setup != NULL) {
                        rc = test->setup(state);
                        /*
                         * Ignore any allocated blocks, just need to ensure they're
                         * deallocated on tear down.
                         */
                } else if (test->teardown != NULL) {
                        rc = test->teardown(state);
                        //fail_if_blocks_allocated(check_point, function_name);
                }
                fail_if_leftover_values();
                global_running_test = 0;
        } else { /* test failed at some place */
                global_running_test = 0;
                rc = -1;
        }
        teardown_testing();

        return rc;
}
static int mockx_run_one_test(struct test_state *state)
{
        int rc;
        const struct unit_test *test;
        struct timeval begin, end, diff;

        assert(state);
        test = state->test;

        state->runtime = 0.0;
        gettimeofday(&begin, NULL);



        rc = mockx_run_one_test_or_fixture(state->test, &state->state);
        if (rc == 0) {
                state->status = CM_TEST_PASSED;
        } else {
                if (global_skip_test) {
                        state->status = CM_TEST_SKIPPED;
                        global_skip_test = 0; /* do not skip the next test */
                } else {
                        state->status = CM_TEST_FAILED;
                }
        }
        rc = 0;




        gettimeofday(&end, NULL);
        timersub(&end, &begin, &diff);

        state->runtime = diff.tv_sec + (double)diff.tv_usec / 1e6;
        state->error_message = global_error_message;
        global_error_message = NULL;

        return 0;
}

int _mockx_run_group_tests(const char *name,
        const struct unit_test *const tests, const int num_tests,
        int (*setup)(void **), int (*teardown)(void **))
{
        int i, rc;
        struct test_state *states, *test;
        double total_runtime = 0.0;
        int total_executed = 0;
        int total_passed = 0;
        int total_failed = 0;
        int total_errors = 0;
        int total_skipped = 0;
        void *group_state = NULL;


#if 0
    const ListNode *group_check_point = check_point_allocated_blocks();
    size_t i;
    int rc;

    /* Make sure LargestIntegralType is at least the size of a pointer. */
    assert_true(sizeof(LargestIntegralType) >= sizeof(void*));
#endif
        states = (struct test_state *)libc_malloc(sizeof(struct test_state) * num_tests);
        if (states == NULL)
                return -1;
        log_info("[==========] Running %u test(s) <%s>.", num_tests, name);

        /* init test array */
        for (i = 0; i < num_tests; i++) {
                states[i].test = &tests[i];
                states[i].status = CM_TEST_NOT_STARTED;
                states[i].state = NULL;
        }

#if 0
    rc = 0;
    /* Run group setup */
    if (group_setup != NULL) {
        rc = cmocka_run_group_fixture("cmocka_group_setup",
                                      group_setup,
                                      NULL,
                                      &group_state,
                                      group_check_point);
    }

    if (rc == 0) {
#endif
        /* Execute tests */
        for (i = 0; i < num_tests; i++) {
                test = &states[i];
                printf_info(PRINTF_TEST_START, test->test->name, NULL);
                if (group_state != NULL)
                        test->state = group_state;

                rc = mockx_run_one_test(test);
                total_executed++;
                total_runtime += test->runtime;
                if (rc == 0) {
                        switch (test->status) {
                        case CM_TEST_PASSED:
                                printf_info(PRINTF_TEST_SUCCESS, test->test->name, NULL);
                                total_passed++;
                                break;
                        case CM_TEST_SKIPPED:
                                printf_info(PRINTF_TEST_SKIPPED, test->test->name, test->error_message);
                                total_skipped++;
                                break;
                        case CM_TEST_FAILED:
                                printf_info(PRINTF_TEST_FAILURE, test->test->name, test->error_message);
                                total_failed++;
                                break;
                        default:
                                printf_info(PRINTF_TEST_ERROR, test->test->name, "internal error");
                                total_errors++;
                                break;
                        }
                } else {
                        printf_info(PRINTF_TEST_ERROR, test->test->name, "couldn't run the test");
                        total_errors++;
                }
        }
#if 0
    } else {
        cmprintf(PRINTF_TEST_ERROR, 0,
                 group_name, "Group setup failed");
        total_errors++;
    }
	    /* Run group teardown */
    if (group_teardown != NULL) {
        rc = cmocka_run_group_fixture("cmocka_group_teardown",
                                      NULL,
                                      group_teardown,
                                      &group_state,
                                      group_check_point);
    }

    cmprintf_group_finish(group_name,
                          total_executed,
                          total_passed,
                          total_failed,
                          total_errors,
                          total_skipped,
                          total_runtime,
                          cm_tests);
#endif

        for (i = 0; i < num_tests; i++)
                if (states[i].error_message != NULL)
                        libc_free((void *)states[i].error_message);
        libc_free(states);
    //fail_if_blocks_allocated(group_check_point, "cmocka_group_tests");

        return total_failed + total_errors;
}
