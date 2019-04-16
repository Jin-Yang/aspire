#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#define SSIZE(arr)  (sizeof(arr) / sizeof(arr[0]))

static int strsplit(char *str, char **fld, int size)
{
        int i;
        char *ptr;

        for (i = 0, ptr = NULL; i < size; i++, str = NULL) {
                fld[i] = strtok_r(str, " \t\r\n", &ptr);
                if (fld[i] == NULL)
                        break;
        }

        return i;
}

#define LOADAVG_MAX_FIELDS   5
int read_loadavg(double load[3])
{
        int fd, rc;
        char buff[128], *fields[5];

        fd = open("/proc/loadavg", O_RDONLY);
        if (fd < 0)
                return -1;

        rc = read(fd, buff, sizeof(buff));
        if (rc < 0) {
                close(fd);
                return -1;
        }
        buff[rc] = 0;
        close(fd);

        rc = strsplit(buff, fields, LOADAVG_MAX_FIELDS);
        if (rc < LOADAVG_MAX_FIELDS)
                return -1;

        load[0] = atof(fields[0]);
        load[1] = atof(fields[1]);
        load[2] = atof(fields[2]);

        return 0;
}
