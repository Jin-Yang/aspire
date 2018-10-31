
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#include <sys/ipc.h>
#include <sys/shm.h>

#define log_it(fmt, args...)  do { printf(fmt, ## args); putchar('\n'); } while(0)

#define MEM_STEP_SIZE   5 * 1024 * 1024

static void exit_usage(int status)
{
        printf("Usage: [OPTIONS]\n\n"
               "Available options:\n"
               "  General:\n"
               "    -s <step>       Sleep N millisecond when try to allocate memory.\n"
               "                    default: 100ms.\n"
               "    -m <size>       N kilo-bytes allocated every time.\n"
               "                    default: 1000KB.\n"
               "    -l <times>      Allocate N times.\n"
               "                    default: 1.\n"
               "    -h              Display help (this message)\n");

        exit(status);
}

static int malloc_libc(int size)
{
        unsigned char *ptr;

        ptr = (unsigned char *)malloc(size * 1024);
        if (ptr == NULL) {
                log_it("[ERROR] malloc memory failed, %s", strerror(errno));
                return -1;
        }
        memset(ptr, 0x55, size * 1024);

        return 0;
}
static int malloc_shm(int size)
{
        int shmid;
        unsigned char *ptr;

        shmid = shmget(IPC_PRIVATE, size * 1024, IPC_CREAT | 0600);
        if (shmid < 0) {
                log_it("[ERROR] shmget memory failed, %s", strerror(errno));
                return -1;
        }

        ptr = (unsigned char *)shmat(shmid, NULL, 0);
        if (ptr == (void *) -1) {
                log_it("[ERROR] shmat failed, %s", strerror(errno));
                return -1;
        }
        memset(ptr, 0x55, size * 1024);

        return 0;
}


int main(int argc, char *argv[])
{
        int i, c;
        int(*getmem)(int) = malloc_libc;
        int sleepn = 100, allocb = 1000, times = 1;

        while (1) {
                c = getopt(argc, argv, "s:m:l:h");
                if (c == -1)
                        break;
                switch (c) {
                case 'l':
                        times = atoi(optarg);
                        break;
                case 'm':
                        allocb = atoi(optarg);
                        break;
                case 's':
                        sleepn = atoi(optarg);
                        break;
                case 'h':
                        exit_usage(EXIT_SUCCESS);
                default:
                        exit_usage(EXIT_FAILURE);
                }
        }

        if (optind < argc) {
                if (strcmp(argv[optind], "libc") == 0) {
                        getmem = malloc_libc;
                } else if (strcmp(argv[optind], "shm") == 0) {
                        getmem = malloc_shm;
                } else {
                        log_it("[ERROR] unsupport method '%s'.", argv[optind]);
                        exit_usage(EXIT_FAILURE);
                }
        }

        log_it("[ INFO] perloop %dKB(alloc) %dms(sleep), try %d times.", allocb, sleepn, times);
        for (i = 1; i <= times; i++) {
                if (getmem(allocb) < 0)
                        return -1;

                log_it("[ INFO] get %dx%dKB size.", i, allocb);
                usleep(sleepn * 1000);
        }
        log_it("[ INFO] done.");

        sleep(10);

        return 0;
}

