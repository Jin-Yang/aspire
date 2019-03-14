#include "testing.h"
#include "histogram.h"

DEF_TEST(simple) {
        struct {
                double val;
                double min;
                double max;
                double sum;
                double avg;
        } cases[] = {
                /* val  min  max  sum   avg */
                { 5, 5,  5,   5,    5},
                { 3, 3,  5,   8,    4},
                { 7, 3,  7,  15,    5},
                {25, 3, 25,  40,   10},
                {99, 3, 99, 139, 27.8},
        };

        size_t i;
        struct histogram *his;

        CHECK_NOT_NULL(his = histogram_create(1));
        for (i = 0; i < sizeof(cases)/sizeof(cases[0]); i++) {
                histogram_add(his, cases[i].val);
                EXPECT_EQ_DOUBLE(cases[i].min, histogram_get_min(his));
                EXPECT_EQ_DOUBLE(cases[i].max, histogram_get_max(his));
                EXPECT_EQ_DOUBLE(cases[i].sum, histogram_get_sum(his));
                EXPECT_EQ_DOUBLE(cases[i].avg, histogram_get_avg(his));
        }

        histogram_destroy(his);
        return 0;
}
DEF_TEST(percentile) {
        int i;
        struct histogram *his;

        CHECK_NOT_NULL(his = histogram_create(1));
        for (i = 1; i <= 100; i++)
                histogram_add(his, i);

        EXPECT_EQ_DOUBLE(1.0, histogram_get_min(his));
        EXPECT_EQ_DOUBLE(100.0, histogram_get_max(his));
        EXPECT_EQ_DOUBLE(5050.0, histogram_get_sum(his));
        EXPECT_EQ_DOUBLE(50.5, histogram_get_avg(his));
        EXPECT_EQ_DOUBLE(1.0, histogram_get_percentile(his, 0));
        EXPECT_EQ_DOUBLE(50.0, histogram_get_percentile(his, 50));
        EXPECT_EQ_DOUBLE(80.0, histogram_get_percentile(his, 80));
        EXPECT_EQ_DOUBLE(95.0, histogram_get_percentile(his, 95));
        EXPECT_EQ_DOUBLE(99.0, histogram_get_percentile(his, 99));
        EXPECT_EQ_DOUBLE(100.0, histogram_get_percentile(his, 100));
        CHECK_ZERO(histogram_get_percentile(his, -1.0));
        CHECK_ZERO(histogram_get_percentile(his, 101.0));

        EXPECT_EQ_INT(1, his->bin_width);
        histogram_add(his, 1001);
        for (i = 0; i < 50; i++)
                EXPECT_EQ_INT(2, his->histogram[i]);
        EXPECT_EQ_INT(1, his->histogram[500]);
        EXPECT_EQ_INT(2, his->bin_width);

        histogram_destroy(his);
        return 0;
}

int main(void)
{
        RUN_TEST(simple);
        RUN_TEST(percentile);

        END_TEST;
}
