#ifndef HISTOGRAM_H_
#define HISTOGRAM_H_

#include <stdio.h>
#include <stdint.h>

#define HISTYPE uint64_t

#ifndef HISTOGRAM_NUM_BINS
#define HISTOGRAM_NUM_BINS 1000
#endif

#if defined __STDC_VERSION__ && __STDC_VERSION__ >= 199901L
        #define log_debug(...) do { printf("debug: " __VA_ARGS__); putchar('\n'); } while(0);
        #define log_info(...)  do { printf(" info: " __VA_ARGS__); putchar('\n'); } while(0);
        #define log_error(...) do { printf("error: " __VA_ARGS__); putchar('\n'); } while(0);
#elif defined __GNUC__
        #define log_debug(fmt, args...)  do { printf("debug: " fmt, ## args); putchar('\n'); } while(0);
        #define log_info(fmt, args...)   do { printf(" info: "  fmt, ## args); putchar('\n'); } while(0);
        #define log_error(fmt, args...)  do { printf("error: " fmt, ## args); putchar('\n'); } while(0);
#endif

struct histogram {
        HISTYPE sum, min, max;
        HISTYPE bin_width, default_bin_with;

        int num;
        int histogram[HISTOGRAM_NUM_BINS];
};

struct histogram *histogram_create(int bin_width);
void histogram_destroy(struct histogram *his);
void histogram_add(struct histogram *his, HISTYPE val);

HISTYPE histogram_get_min(struct histogram *his);
HISTYPE histogram_get_max(struct histogram *his);
HISTYPE histogram_get_sum(struct histogram *his);
double histogram_get_avg(struct histogram *his);
int histogram_get_num(struct histogram *his);
HISTYPE histogram_get_percentile(struct histogram *his, double percent);

#endif
