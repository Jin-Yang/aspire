
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <dlfcn.h>
#include <errno.h>
#include <stdio.h>
#include <stdarg.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>

#define log_it(fmt, args...)  do { printf(fmt, ## args); putchar('\n'); } while(0)

#if 0
        // delete evironment.
        int i;
        char *ptr;
        char **tenvp = (char **)envp;
        for (i = 0; tenvp[i] != NULL; i++) {
                ptr = strchr(tenvp[i], '=');
                if (ptr == NULL)
                        continue;

                if (strncmp("LD_PRELOAD", tenvp[i], ptr - tenvp[i]) == 0) {
                        printf("    envp[%02d] %s\n", i, tenvp[i]);
                        break;
                }
        }

        for (; envp[i] != NULL; i++)
                tenvp[i] = tenvp[i + 1];
#endif


/*
 * Avoid dynamic memory allocation due two main issues:
 *   1. The function should be async-signal-safe and a running on a signal
 *      handler with a fail outcome might lead to malloc bad state.
 *   2. It might be used in a vfork/clone(VFORK) scenario where using
 *      malloc also might lead to internal bad state.
 */
#define convert_vec()                                              \
        int argc;                                                  \
        for (argc = 1; va_arg(ap, const char *); argc++) {         \
                if (argc == INT_MAX) {                             \
                        va_end(ap);                                \
                        errno = E2BIG;                             \
                        return -1;                                 \
                }                                                  \
        }                                                          \
        va_end(ap);                                                \
                                                                   \
        int i;                                                     \
        char *argv[argc + 1];                                      \
        va_start(ap, arg);                                         \
        argv[0] = (char *)arg;                                     \
        for (i = 1; i <= argc; i++)                                \
                argv[i] = va_arg(ap, char *);                      \

#define get_next_symbol(type, var, name) do {      \
        var = (type)dlsym(RTLD_NEXT, name);        \
        if (var == NULL) {                         \
                errno = ENOSYS;                    \
                log_it("failed to find " name);    \
                return -1;                         \
        }                                          \
} while(0)

typedef int (*texecve)(const char *path, char *const argv[], char *const envp[]);
int execve(const char *path, char *const argv[], char *const envp[])
{
        texecve oexecve;

        log_it("<<< execve >>> %s", path);
        get_next_symbol(texecve, oexecve, "execve");

        return (*oexecve)(path, argv, envp);
}

typedef int (*texecv)(const char *path, char *const argv[]);
int execv(const char *path, char *const argv[])
{
        texecv oexecv;

        log_it("<<<  execv >>> %s", path);
        get_next_symbol(texecv, oexecv, "execv");

        return oexecv(path, argv);
}

typedef int (*texecvp)(const char *path, char *const argv[]);
int execvp(const char *file, char *const argv[])
{
        texecvp oexecvp;

        log_it("<<< execvp >>> %s", file);
        get_next_symbol(texecvp, oexecvp, "execvp");

        return oexecvp(file, argv);
}

int execle(const char *path, const char *arg, ...)
{
        va_list ap;
        char **envp;
        texecve oexecve;

        log_it("<<< execle >>> %s", path);
        get_next_symbol(texecve, oexecve, "execve");

        va_start(ap, arg);
        convert_vec();
        envp = va_arg(ap, char **);
        va_end(ap);

        return oexecve(path, argv, envp);
}

int execl(const char *path, const char *arg, ...)
{
        va_list ap;
        texecv oexecv;

        log_it("<<<  execl >>> %s", path);
        get_next_symbol(texecv, oexecv, "execv");

        va_start(ap, arg);
        convert_vec();
        va_end(ap);

        return oexecv(path, argv);
}

int execlp(const char *file, const char *arg, ...)
{
        va_list ap;
        texecvp oexecvp;

        log_it("<<< execlp >>> %s", file);
        get_next_symbol(texecvp, oexecvp, "execvp");

        va_start(ap, arg);
        convert_vec();
        va_end(ap);

        return oexecvp(file, argv);
}
