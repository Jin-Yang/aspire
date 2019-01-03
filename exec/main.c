
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#include <errno.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/prctl.h>

#include "abuff.h"
#include "libev/ev.h"

#define EXEC_MIN_BUFFER   4096
#define EXEC_MAX_BUFFER   4096 * 16

struct exec_context {
        ev_timer wtimeout;
        ev_io    wout, werr;
        ev_child wchild;
        int count; /* timeout */
        struct abuff *outbuf;
};

#if defined __STDC_VERSION__ && __STDC_VERSION__ >= 199901L
        #define _debug(...) do { printf("debug: " __VA_ARGS__); putchar('\n'); } while(0);
        #define _info(...)  do { printf(" info: " __VA_ARGS__); putchar('\n'); } while(0);
        #define _error(...) do { printf("error: " __VA_ARGS__); putchar('\n'); } while(0);
#elif defined __GNUC__
        #define _debug(fmt, args...)  do { printf("debug: " fmt, ## args); putchar('\n'); } while(0);
        #define _info(fmt, args...)   do { printf(" info: "  fmt, ## args); putchar('\n'); } while(0);
        #define _error(fmt, args...)  do { printf("error: " fmt, ## args); putchar('\n'); } while(0);
#endif

#define exec_close_evfd(w) do { \
        ev_io_stop(EV_A_ (w));  \
        if ((w)->fd > 0) {      \
                close((w)->fd); \
                (w)->fd = -1;   \
        }                       \
} while(0)

void exec_destory_context(struct exec_context *ctx)
{
        if (ctx == NULL)
                return;

        ev_child_stop(EV_A_ &ctx->wchild);
        ev_timer_stop(EV_A_ &ctx->wtimeout);
        exec_close_evfd(&ctx->werr);
        exec_close_evfd(&ctx->wout);

        abuff_destory(ctx->outbuf);

        free(ctx);
}

void exec_close_pipe(int fd_pipe[2])
{
        if (fd_pipe[0] != -1)
                close(fd_pipe[0]);

        if (fd_pipe[1] != -1)
                close(fd_pipe[1]);
}


void exec_child_exec(void)
{
        char *const argv[] = {(char *)"bash", (char *)"-c", (char *)"/tmp/test.sh", NULL};

        /* setgid() setuid() setegid() setgroups() */

        /* char *const argv[] = {(char *)"ls", (char *)"-l", NULL};
        execvp("ls", argv); */

        execvp("bash", argv);

        _error("execvp error, %s", strerror(errno));
}

int exec_child_fork(int *fd_in, int *fd_out, int *fd_err)
{
        int fdp_in[2] = {-1, -1};
        int fdp_out[2] = {-1, -1};
        int fdp_err[2] = {-1, -1};

        int fd, fdnums;
        pid_t pid;

        if (pipe(fdp_in) < 0 || pipe(fdp_out) < 0 || pipe(fdp_err) < 0) {
                _error("failed to create pipe, %s", strerror(errno));
                goto failed;
        }

        /* check if user exists */

        pid = fork();
        if (pid < 0) {
                _error("fork failed, %s", strerror(errno));
                goto failed;
        } else if (pid == 0) {
                /* destory all struct if needed */
                ev_break(EVBREAK_ALL);
		setpgrp();
		prctl(PR_SET_PDEATHSIG, SIGKILL);

                /* Close all file descriptors but the pipe end we need. */
                fdnums = getdtablesize();
                for (fd = 0; fd < fdnums; fd++) {
                        if ((fd == fdp_in[0]) || (fd == fdp_out[1]) || (fd == fdp_err[1]))
                                continue;
                        close(fd);
                }

                /* Connect the 'in' pipe to STDIN */
                if (fdp_in[0] != STDIN_FILENO) {
                        dup2(fdp_in[0], STDIN_FILENO);
                        close(fdp_in[0]);
                }

                /* Connect the 'out' pipe to STDOUT */
                if (fdp_out[1] != STDOUT_FILENO) {
                        dup2(fdp_out[1], STDOUT_FILENO);
                        close(fdp_out[1]);
                }

                /* Connect the 'err' pipe to STDERR */
                if (fdp_err[1] != STDERR_FILENO) {
                        dup2(fdp_err[1], STDERR_FILENO);
                        close(fdp_err[1]);
                }


                /* set_environment(); */

                /* Unblock all signals */
                /* reset_signal_mask(); */

                exec_child_exec();

                /* clean up, or valgrind complain reachable
                exec_destory_context(ctx);
                ev_loop_destroy(EV_A_); */
                /* does not return */
                exit(0);
        }

        close(fdp_in[0]);
        close(fdp_out[1]);
        close(fdp_err[1]);

        if (fd_in != NULL)
                *fd_in = fdp_in[1];
        else
                close(fdp_in[1]);

        if (fd_out != NULL)
                *fd_out = fdp_out[0];
        else
                close(fdp_out[0]);

        if (fd_err != NULL)
                *fd_err = fdp_err[0];
        else
                close(fdp_err[0]);

        return pid;

failed:
        exec_close_pipe(fdp_in);
        exec_close_pipe(fdp_out);
        exec_close_pipe(fdp_err);

        return -1;
}

#define READ_RETRY   -3
#define READ_EOF     -4
#define READ_FATAL   -5

#define read_wrap(b, l) do {                                                 \
        len = read(fd, b, l);                                                \
        if (len < 0) {                                                       \
                if (errno == EAGAIN || errno == EINTR)                       \
                        return READ_RETRY;                                   \
                _error("Read from #%d failed, %s\n", fd, strerror(errno));   \
                return READ_FATAL; /* maybe not fatal */                     \
        } else if (len == 0) {                                               \
                return READ_EOF;                                             \
        }                                                                    \
        _debug("Read from %d with %d bytes.", fd, len);                      \                                                                \
} while(0)

static int exec_buffer_read(int fd, struct abuff **buffer)
{
        struct abuff *buf;
        char dropbuf[EXEC_MIN_BUFFER];
        int len = 0;

        assert(buffer);
        buf = *buffer;

        if (buf == NULL) {
                /* try to read some bytes at first time */
                read_wrap(dropbuf, sizeof(dropbuf));
                buf = abuff_new(EXEC_MIN_BUFFER, EXEC_MAX_BUFFER);
                if (buf == NULL) {
                        _error("Create new buffer failed, out of memory");
                        return READ_FATAL;
                }
                *buffer = buf;
                len = abuff_append(buf, dropbuf, len);
                if (len < 0) {
                        _error("Expand buffer failed, out of memory");
                        return READ_FATAL;
                }
                return len;
        } else if (abuff_is_full(buf)) {
                read_wrap(dropbuf, sizeof(dropbuf));
                _error("Read buffer is full now, drop %d bytes", len);
                return len;
        } else if (abuff_left(buf) < 128) {
                len = abuff_exponent_expand(buf);
                if (len < 0) {
                        _error("Expand buffer failed, out of memory");
                        return READ_FATAL;
                }
                assert(len > 0);
        } else {
		len = abuff_length(buf);
	}

        read_wrap(buf->tail, len);
        buf->tail += len;

        return len;
}

void exec_timeout_cb(EV_P_ ev_timer *w, int revents)
{
        (void) w;
        (void) revents;
        struct exec_context *ctx;

        ev_timer_stop(w);
        ctx = (struct exec_context *)w->data;
        if (++ctx->count >= 3) {
                _error("Timeout %d times, force quit", ctx->count);
                kill(-ctx->wchild.pid, SIGKILL);
        } else {
                _error("Timeout %d times, kill it", ctx->count);
                kill(-ctx->wchild.pid, SIGINT);
                ev_timer_set(w, 3., 0.);
                ev_timer_start(w);
        }
}

void exec_stdout_cb(EV_P_ ev_io *w, int revents)
{
        (void) revents;
        int rc;
        struct exec_context *ctx;

        ctx = (struct exec_context *)w->data;

        rc = exec_buffer_read(w->fd, &ctx->outbuf);
        if (rc == READ_EOF) {
                _info("STDOUT: Read end of file");
                exec_close_evfd(w);
                return;
        } else if (rc == READ_FATAL) {
                _error("STDOUT: Got a fatal error, kill %d", ctx->wchild.pid);
                exec_close_evfd(w);
		if (ctx->wchild.pid > 1)
                	kill(-ctx->wchild.pid, SIGKILL);
                return;
        }
}



void exec_stderr_cb(EV_P_ ev_io *w, int revents)
{
        (void) revents;
        char buffer[1024];
        int len;

        len = read(w->fd, buffer, sizeof(buffer) - 1);
        if (len < 0) {
                if (errno == EAGAIN || errno == EINTR)
                        return; /* retry later */
                _error("STDERR: Read from #%d failed, %s", w->fd, strerror(errno));
                exec_close_evfd(w);
                return;
        } else if (len == 0) {
                _info("STDERR: Read end of file");
                exec_close_evfd(w);
                return;
        }
        buffer[len] = 0;

        _debug("STDERR: %s", buffer);
}

void exec_child_cb(EV_P_ ev_child *w, int revents)
{
        (void) revents;
	int rc;
        struct exec_context *ctx;

        ctx = w->data;
        _info("Process %d exited with status %x", w->rpid, w->rstatus);
	
	exec_close_evfd(&ctx->werr);
	ev_timer_stop(&ctx->wtimeout);
	ev_child_stop(&ctx->wchild);
	ctx->wchild.pid = -1;
	
	if (ctx->wout.fd > 0) {
		do {
			rc = exec_buffer_read(ctx->wout.fd, &ctx->outbuf);
		} while(rc > 0 || rc == READ_RETRY);
	}
	exec_close_evfd(&ctx->wout);
	
        if (ctx->outbuf) {
                abuff_seal(ctx->outbuf);
                _info("Got %d bytes >>>>>>>>\n%s\n<<<<<<<<",
                        abuff_length(ctx->outbuf), abuff_string(ctx->outbuf));
        }

        exec_destory_context(ctx);
}

struct exec_context *exec_start_job(void)
{
        int fd_out, fd_err;
        int rc;
        struct exec_context *ctx;

        ctx = (struct exec_context *)calloc(1, sizeof(struct exec_context));
        if (ctx == NULL) {
                fprintf(stderr, "failed calloc context, out of memory");
                return NULL;
        }

        rc = exec_child_fork(NULL, &fd_out, &fd_err);
        if (rc < 0) {
                free(ctx);
                return NULL;
        }

        ev_io_init(&ctx->wout, exec_stdout_cb, fd_out, EV_READ);
        ctx->wout.data = ctx;
        ev_io_start(EV_A_ &ctx->wout);

        ev_io_init(&ctx->werr, exec_stderr_cb, fd_err, EV_READ);
        ev_io_start(EV_A_ &ctx->werr);

        ev_timer_init(&ctx->wtimeout, exec_timeout_cb, 3., 0.);
        ctx->wtimeout.data = ctx;
        ev_timer_start(EV_A_ &ctx->wtimeout);

        ev_child_init(&ctx->wchild, exec_child_cb, rc, 0);
        ctx->wchild.data = ctx;
        ev_child_start(EV_A_ &ctx->wchild);

        return ctx;
}

int main(void)
{
        EV_P EV_DEFAULT;

        /* Become reaper of our children */
        if (prctl(PR_SET_CHILD_SUBREAPER, 1) < 0) {
                _error("failed to make us a subreaper, %s.", strerror(errno));
                if (errno == EINVAL)
                        _error("Perhaps the kernel version is too old (< 3.4?).");
        }

        exec_start_job();

        ev_run(EV_A_ 0);

        ev_loop_destroy(EV_A_);

        return 0;
}

