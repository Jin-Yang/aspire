
#include "histogram.h"

#include <limits.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>

#define MOD "(hist) "

/*
 * Histogram represents the distribution of data, it has a list of "bins".
 * Each bin represents an interval and has a count (frequency) of number of
 * values fall within its interval.
 *
 * Histogram's range is determined by the number of bins and the bin width.
 * There are 1000 bins and all bins have the same width of default 1 millisecond.
 * So the default range is 1 second. When a value above this range is added,
 * histogram's range is increased by increasing the bin width (NOTE that number
 * of bins remains always at 1000).
 *
 * The operation of increasing bin width is little expensive as each bin need
 * to be visited to update it's count. To reduce frequent change of bin width,
 * new bin width will be the next nearest power of 2. Example: 2, 4, 8, 16, 32,
 * 64, 128, 256, 512, 1024, 2048, 4096, ...
 *
 * So, if the required bin width is 300, then new bin width will be 512 as it is
 * the next nearest power of 2.
 */
static HISTYPE inline next_pow2(HISTYPE v)
{
        HISTYPE p = 1;

        if (v && !(v & (v - 1)))
                return v;
        while (p < v)
                p <<= 1;
        return p;
}

static void change_bin_width(struct histogram *his, HISTYPE val)
{
        int i, times, new_bin;
        HISTYPE nbw, obw; /* new & old bin width */

        obw = his->bin_width;
        nbw = next_pow2((val - 1) / HISTOGRAM_NUM_BINS + 1);
        if (nbw == obw) {
                log_error(MOD "change bin width failed, old(%ld) new(%ld).", obw, nbw);
                return;
        }
        his->bin_width = nbw;

        /* iterate through all bins and move the old bin's count to new bin. */
        if (his->num > 0) {
                times = nbw / obw;
                for (i = 0; i < HISTOGRAM_NUM_BINS; i++) {
                        new_bin = i / times;
                        if (i == new_bin)
                                continue;
                        assert(new_bin < i);
                        his->histogram[new_bin] += his->histogram[i];
                        his->histogram[i] = 0;
                }
        }
        log_debug(MOD "change bin width, value(%ld) old_bin_width(%ld) new_bin_width(%ld).",
                        val, obw, nbw);
}

void histogram_add(struct histogram *his, HISTYPE val)
{
        int bin;

        if (his == NULL || val == 0 || val > LLONG_MAX)
                return;
        his->sum += val;
        his->num++;

        if (his->min == 0 && his->max == 0)
                his->min = his->max = val;
        if (his->min > val)
                his->min = val;
        if (his->max < val)
                his->max = val;

        bin = (val - 1) / his->bin_width;
        if (bin >= HISTOGRAM_NUM_BINS) {
                change_bin_width(his, val); /* change and retry */
                bin = (val - 1) / his->bin_width;
                if (bin >= HISTOGRAM_NUM_BINS) {
                        log_error(MOD "histogram add failed, invalid bin %d.", bin);
                        return;
                }
        }
        his->histogram[bin]++;
}

void histogram_reset(struct histogram *his)
{
        int bin_width, max_bin;

        if (his == NULL)
                return;
        bin_width = his->bin_width;
        max_bin = (his->max - 1) / his->bin_width;

        if ((his->num > 0) && (his->bin_width >= his->default_bin_with * 2) &&
                (max_bin < HISTOGRAM_NUM_BINS / 4)) {
                bin_width = bin_width / 2;
                log_debug(MOD "histogram reset, max(%ld) max_bin(%d) old_bin_width(%ld) "
                        "new_bin_width(%d).", his->max, max_bin, his->bin_width, bin_width);
        }
        memset(his, 0, sizeof(*his));
        his->bin_width = bin_width;
}

struct histogram *histogram_create(int bin_width)
{
        struct histogram *his;

        if (bin_width < 0)
                return NULL;

        his = (struct histogram *)calloc(1, sizeof(*his));
        if (his == NULL)
                return NULL;
        his->default_bin_with = bin_width;
        his->bin_width = bin_width;

        return his;
}

void histogram_destroy(struct histogram *his)
{
        if (his == NULL)
                return;
        free(his);
}

int histogram_get_num(struct histogram *his)
{
        if (his == NULL)
                return 0;
        return his->num;
}

HISTYPE histogram_get_min(struct histogram *his)
{
        if (his == NULL)
                return 0;
        return his->min;
}
HISTYPE histogram_get_max(struct histogram *his)
{
        if (his == NULL)
                return 0;
        return his->max;
}

HISTYPE histogram_get_sum(struct histogram *his)
{
        if (his == NULL)
                return 0;
        return his->sum;
}

double histogram_get_avg(struct histogram *his)
{
        if (his == NULL || his->num == 0)
                return 0;
        return (double)his->sum / (double)his->num;
}

/* TP90 TP99 used mostly. */
HISTYPE histogram_get_percentile(struct histogram *his, double percent)
{
        int sum = 0, i;
        double p, percent_upper = 100.0, percent_lower = .0;

        if (his == NULL || his->num == 0 || percent < 0.0 || percent > 100.0)
                return 0;
        if (percent >= 100.0)
                return his->max;
        if (percent <= .0)
                return his->min;

        percent = 100. - percent;
        for (i = HISTOGRAM_NUM_BINS - 1; i > 0; i--) {
                percent_lower = percent_upper;
                sum += his->histogram[i];
                if (sum == 0)
                        percent_upper = 0.0;
                else
                        percent_upper = 100.0 * ((double)sum) / ((double)his->num);
                if (percent_upper >= percent)
                        break;
        }

        if (i >= HISTOGRAM_NUM_BINS)
                return 0;
        assert(percent_upper >= percent);
        assert(percent_lower < percent);
        if (i == 0)
                return his->bin_width;

        //log_debug(MOD "get percentile, i %d lower %.2f upper %.2f", i, percent_lower, percent_upper);
        p = (percent_upper - percent) / (percent_upper - percent_lower);
        return i * his->bin_width + p * his->bin_width;
}
