#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/xattr.h>

#define SHORT_LABEL_LEN 23
#define SMACK_LABEL_LEN 255
#define DICT_HASH_SIZE 4096

#ifndef SO_PEERSEC
#define SO_PEERSEC 31
#endif

static inline ssize_t get_label(char *dest, const char *src, unsigned int *hash)
{
    int i;
    unsigned int h = 5381;/*DJB2 hashing function magic number*/;

    if (!src || src[0] == '\0' || src[0] == '-')
        return -1;

    for (i = 0; i < (SMACK_LABEL_LEN + 1) && src[i]; i++) {
        if (src[i] <= ' ' || src[i] > '~')
            return -1;
        switch (src[i]) {
            case '/':
            case '"':
            case '\\':
            case '\'':
                return -1;
            default:
                break;
        }

        if (dest)
            dest[i] = src[i];
        if (hash)
            /* This efficient hash function,
             * created by Daniel J. Bernstein,
             * is known as DJB2 algorithm */
            h = (h << 5) + h + src[i];
    }

    if (dest && i < (SMACK_LABEL_LEN + 1))
        dest[i] = '\0';
    if (hash)
        *hash = h % DICT_HASH_SIZE;

    return i < (SMACK_LABEL_LEN + 1) ? i : -1;
}

ssize_t smack_new_label_from_socket(int fd, char **label)
{
    char dummy;
    int ret;
    socklen_t length = 1;
    char *result;

    ret = getsockopt(fd, SOL_SOCKET, SO_PEERSEC, &dummy, &length);
    if (ret < 0 && errno != ERANGE)
        return -1;

    result = calloc(length + 1, 1);
    if (result == NULL)
        return -1;

    ret = getsockopt(fd, SOL_SOCKET, SO_PEERSEC, result, &length);
    if (ret < 0) {
        free(result);
        return -1;
    }

    *label = result;
    return length;
}

ssize_t smack_new_label_from_path(const char *path, const char *xattr,
                                  int follow, char **label)
{
    char buf[SMACK_LABEL_LEN + 1];
    char *result;
    ssize_t ret = 0;

    ret = follow ?
          getxattr(path, xattr, buf, SMACK_LABEL_LEN + 1) :
          lgetxattr(path, xattr, buf, SMACK_LABEL_LEN + 1);
    if (ret < 0)
        return -1;
    buf[ret] = '\0';

    result = calloc(ret + 1, 1);
    if (result == NULL)
        return -1;

    ret = get_label(result, buf, NULL);
    if (ret < 0) {
        free(result);
        return -1;
    }

    *label = result;
    return ret;
}

int main(int cn, char **cv){
    return 0;
}
