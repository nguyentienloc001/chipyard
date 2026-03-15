// See LICENSE.Sifive for license details.

#ifndef _SIFIVE_CLINT_H
#define _SIFIVE_CLINT_H


#define CLINT_MSIP_BASE     0x0000
#define CLINT_MSIP_size     0x4
#define CLINT_MTIMECMP      0x4000
#define CLINT_MTIMECMP_size 0x8
#define CLINT_MTIME         0xBFF8
#define CLINT_MTIME_size    0x8

#define CLINT_MSIP0         CLINT_MSIP_BASE + 0x0
#define CLINT_MSIP1         CLINT_MSIP_BASE + 0x4
#define CLINT_MSIP2         CLINT_MSIP_BASE + 0x8
#define CLINT_MSIP3         CLINT_MSIP_BASE + 0xC

#define CLINT_MSIPEN    0x1
#define CLINT_MSIPCLR   0x0

#endif /* _SIFIVE_CLINT_H */
