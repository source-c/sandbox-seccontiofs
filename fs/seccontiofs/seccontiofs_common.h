#ifndef _SECCONTIOFS_COMMON_H
#define _SECCONTIOFS_COMMON_H

#if defined(BSD) || defined(__APPLE__)
#include <stdint.h>
typedef uint8_t __u8;
typedef uint16_t __u16;
typedef uint32_t __u32;
typedef uint64_t __u64;
#else

#include <linux/types.h>    // linux kernel/ASM types

#endif

/* some basic definitions */

#define SECCONTIOFS_PRIV_LBL "P1"
#define SECCONTIOFS_UNPRIV_LBL "U1"
#define SECCONTIOFS_LABEL_LEN 2
#define SECCONTIOFS_WRITABLE_MODE -1
#define SECCONTIOFS_PRIV_CG_NAME "/lxc/cont-priv"
#define SECCONTIOFS_PRIV_CG_NAME_LEN 14

// Add __attribute__((packed)) if compiler supports it
// because some gcc versions (at least ARM) lack support of #pragma pack()

#ifdef HAVE_ATTR_PACKED
#define ATTR_PACKED __attribute__((packed))
#else
#define ATTR_PACKED
#endif

typedef struct {
    __u8 len;
    __u8 type;
    unsigned char payload[14];
} ATTR_PACKED sciomsg;

#define SECCONTIOFS_IOCTL_MAGIC   'x'

#define SECCONTIOFS_IOCTL_IOMSG   _IOW(SECCONTIOFS_IOCTL_MAGIC, 0x8F, sciomsg*)


#endif //_SECCONTIOFS_COMMON_H
