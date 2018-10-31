#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>

#define log_it(fmt, args...)  do { printf(fmt, ## args); putchar('\n'); } while(0)
struct fincore {
	long cached;
};

static void exit_usage(int status)
{
        printf("Usage: fincore [OPTIONS] files\n\n"
		"Available Options:\n"
		"    -h                Display help (this message).\n\n"
		"Compile: " __DATE__ " "  __TIME__ "\n");

	exit(status);
}

int show_fincore(char *fname, struct fincore *res)
{
	int fd;
	struct stat stat;
	void *fmap = NULL;
	unsigned char *vec = NULL;        /* vector result from mincore */
	size_t page_size, file_pages, pages_in_core, i;

	/* the oldest page in the cache or -1 if it isn't in the cache. */
	off_t min_cached_page = -1 ;

	page_size = getpagesize(); /* OR sysconf(_SC_PAGE_SIZE) */

	fd = open(fname, O_RDONLY);
	if (fd == -1) {
		log_it("[ERROR] Could not open file(%s), %s.",
				fname, strerror(errno));
		return -1;
	}

	if (fstat(fd, &stat) < 0) {
		log_it("[ERROR] Stat file(%s) failed, %s.",
				fname, strerror(errno));
		goto cleanup;
	}

        /* ignore empty file */
	if (stat.st_size == 0)
		goto cleanup;

	file_pages = (stat.st_size + page_size - 1) / page_size;
	vec = malloc(file_pages);
	if (vec == NULL) {
		log_it("[ERROR] malloc vector result failed, out of memory.");
		goto cleanup;
	}

	fmap = mmap(NULL, stat.st_size, PROT_NONE, MAP_SHARED, fd, 0); /* PROT_READ */
	if (fmap == MAP_FAILED) {
		log_it("[ERROR] mmap file(%s) failed, %s.",
				fname, strerror(errno));
		goto cleanup;
	}

	if (mincore(fmap, stat.st_size, vec) != 0 ) {
		log_it("[ERROR] mincore file(%s) failed, %s.",
				fname, strerror(errno));
		goto cleanup;
	}

	pages_in_core = 0;
	for (i = 0; i < file_pages; i++) {
		if (vec[i] & 1) {
			pages_in_core++;
			if (min_cached_page == -1 || (off_t)i < min_cached_page)
				min_cached_page = i;
		}
	}

	log_it("[ INFO] %s", fname);
	log_it("[ INFO] size: %ld", stat.st_size);
	log_it("[ INFO] block size: %ld", stat.st_blksize);
	log_it("[ INFO] total pages: %ld", file_pages);
	log_it("[ INFO] min cached page: %ld", min_cached_page);
	log_it("[ INFO] cached: %ld", pages_in_core);
	log_it("[ INFO] cached size: %ld", pages_in_core * page_size);
	log_it("[ INFO] cached percent: %.2f", pages_in_core * 100.0 / file_pages);
	res->cached = min_cached_page;

	munmap(fmap, stat.st_size);
	free(vec);
	close(fd);
	return 0;

cleanup:
	if (fmap)
		munmap(fmap, stat.st_size);
	if (vec)
		free(vec);
	close(fd);
	return -1;
}

int main(int argc, char *argv[])
{
	int rc;
	struct fincore res;
	long total_size = 0;

        while (1) {
                rc = getopt(argc, argv, "h");
                if (rc == -1)
                        break;
                switch (rc) {
			/*
                case 'l':
                        loglevel = optarg;
                        break;
			*/
                case 'h':
                        exit_usage(EXIT_SUCCESS);
                default:
                        exit_usage(EXIT_FAILURE);
                }
        }

	while (optind < argc) {
		if (show_fincore(argv[optind++], &res) < 0)
			break;
		total_size += res.cached;
	}

	return 0;
}


