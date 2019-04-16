#include "load.h"
#include <stdio.h>

int main(void)
{
        double load[3];

        if (read_loadavg(load) < 0) {
                fprintf(stderr, "read load average failed.\n");
                return -1;
        }
        printf("get avg[1]=%.2f avg[5]=%.2f avg[15]=%.2f\n", load[0], load[1], load[2]);

        return 0;
}
