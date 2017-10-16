#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "../fs/seccontiofs/seccontiofs_common.h"

int main(int cn, char **cv)
{
    int fd, ret = -1;

    if (cn != 2)
        return -1;

    fd = open(cv[1], O_RDONLY);

    if (fd < 0) {
        perror("open:");
        return 1;
    }

    {
        sciomsg iomsg = { .len = 0, .type = 1, .payload = {0xaa} };
        ret = ioctl(fd, SECCONTIOFS_IOCTL_IOMSG, &iomsg);
    }

    if (ret < 0)
        perror("ioctl");

    return 0;
}
