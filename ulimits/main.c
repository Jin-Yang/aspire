
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <fcntl.h>
#include <sys/stat.h>

#include <sys/mman.h>

#include <signal.h>

#include <mqueue.h>
#include <sys/wait.h>

#include <math.h>

#define log_info(...)     do { printf("info : " __VA_ARGS__); putchar('\n'); } while(0);
#define log_error(...)    do { printf("error: " __VA_ARGS__); putchar('\n'); } while(0);

void exit_usage(int exitcode)
{
	exit(exitcode);
}

void waste_file_number(void)
{
        int i, rc;

        for (i = 0; i < 4096; i++) {
                rc = open("/tmp/foobar.txt", O_CREAT | O_RDONLY, S_IRWXU | S_IRGRP | S_IROTH);
                if (rc < 0) {
                        log_error("open file failed, %s.", strerror(errno));
                        return;
                }
                log_info("#%04d files opened.", i);
                usleep(100 * 1000);
        }
}

void waste_memory_lock(void)
{
        char arr[4096];

        if (mlock((const void *)arr, sizeof(arr)) < 0) {
                log_error("lock memory failed, %s.", strerror(errno));
                return;
        }
        log_info("success to lock stack memory at %p len %zd.", arr, sizeof(arr));

        if (munlock((const void *)arr, sizeof(arr)) < 0) {
                log_error("unlock memory failed, %s.", strerror(errno));
                return;
        }
        log_info("success to unlock stack memory at %p len %zd.", arr, sizeof(arr));
}

void signal_handler(int sig)
{
        log_info("handle signal %d.", sig);
}
void waste_pending_signals(void)
{
        int i;
        pid_t pid;
        sigset_t newset, oldset;

        signal(SIGRTMIN, signal_handler);
        sigfillset(&newset);
        sigprocmask(SIG_BLOCK, &newset, &oldset);

        pid = fork();
        if (pid < 0) {
                log_error("fork failed, %s.", strerror(errno));
                return;
        } else if (pid == 0) {
                for (i = 0; i < 3; i++){
                        kill(getppid(), SIGRTMIN);
                        log_info("signal %d sent from child.", SIGRTMIN);
                }
                exit(EXIT_FAILURE);
        }
        log_info("parent sleep 1 seconds.");
        sleep(1);
        log_info("parent wake up.");
        sigprocmask(SIG_SETMASK, &oldset, NULL);
        sleep(1);
        log_info("exit.");
}

struct message {
        char mtext[128];
};

int send_message(mqd_t qid, int pri, const char *msg)
{
        if (mq_send(qid, msg, strlen(msg) + 1, pri) < 0) {
                log_error("send POSIX message failed, %s.", strerror(errno));
                return -1;
        }
        return 0;
}
void waste_posix_mq(void)
{
        int rc;
        pid_t pid;
        mqd_t mqid;
        struct mq_attr mattr = {
                .mq_maxmsg = 10,
                .mq_msgsize = sizeof(struct message)
        };

        mqid = mq_open("/foobar_mq", O_CREAT | O_RDWR, S_IREAD | S_IWRITE, &mattr);
        if (mqid < 0) {
                log_error("open POSIX message queue failed, %s.", strerror(errno));
                return;
        }

        pid = fork();
        if (pid < 0) {

        } else if (pid == 0) {
                send_message(mqid, 1, "This is first message.");
                send_message(mqid, 2, "This is second message.");
                send_message(mqid, 3, "No more messages.");
                mq_close(mqid);
                exit(0);
        }
        wait(NULL);

        do {
                u_int pri;
                struct message msg;
                rc = mq_receive(mqid, (char *)&msg, sizeof(msg), &pri);
                if (rc < 0) {
                        log_error("receive POSIX message queue failed, %s.", strerror(errno));
                        break;
                }
                log_info("got message pri(%d) len(%d): %s", pri, rc, msg.mtext);

                rc = mq_getattr(mqid, &mattr);
                if (rc < 0) {
                        log_error("get POSIX message queue attr failed, %s.",
                                strerror(errno));
                        break;
                }
        } while(mattr.mq_curmsgs);

        mq_close(mqid);
        mq_unlink("/foobar_mq");
}
void waste_cpu_time(void)
{
        while(1) {
                sqrt(M_PI);
                //usleep(10);
        }
}


void waste_user_process(void)
{
        int i;
        pid_t pid;

        for(i = 0; i < 100; i++) {
                pid = fork();
                if (pid < 0) {
                        log_error("fork #%d failed, %s.",
                                getpid(), strerror(errno));
                        exit(1);
                } else if (pid == 0) {
			sleep(100);
		}
                log_info("fork #%d process %d.", i, pid);
        }
}

int main(int argc, char **argv)
{
        int c;

        opterr = 0;
        while(1) {
                c = getopt(argc, argv, "hnliqtu");
                if (c == -1)
                        break;

                switch(c) {
                case 'n':
                        waste_file_number();
                        break;
                case 'l':
                        waste_memory_lock();
                        break;
                case 'i':
                        waste_pending_signals();
                        break;
                case 'q':
                        waste_posix_mq();
                        break;
                case 't':
                        waste_cpu_time();
                        break;
                case 'u':
                        waste_user_process();
                        break;
                case 'h':
                        exit_usage(EXIT_SUCCESS);
                case '?':
                default:
                        fprintf(stderr, "unknown option '%c', check usage with '-h'.\n", optopt);
                        exit(EXIT_FAILURE);
                }
        }

        return 0;
}
