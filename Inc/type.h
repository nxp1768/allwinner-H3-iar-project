
#ifndef _SUNXI_TYPE_H_
#define _SUNXI_TYPE_H_

typedef unsigned char      uchar;
typedef unsigned char      u8;
typedef signed char        s8;
typedef unsigned short     u16;
typedef signed short       s16;
typedef unsigned int       u32;
typedef unsigned int       uint;
typedef signed int         s32;
typedef unsigned long long u64;
typedef unsigned long long ulong;

typedef signed char __s8;
typedef unsigned char __u8;

typedef signed short __s16;
typedef unsigned short __u16;

typedef signed int __s32;
typedef unsigned int __u32;

//#undef size_t
typedef unsigned int size_t;

#define     __O     volatile
#define     __IO    volatile

#undef NULL
#if defined(__cplusplus)
#define NULL 0
#else
#define NULL ((void *)0)
#endif


#define readl(addr) (*(volatile u32 *) (addr))
#define writel(val, addr) ((*(volatile u32 *) (addr)) = (val))

#define readb(addr) (*(volatile u8 *) (addr))
#define writeb(val, addr) ((*(volatile u8 *) (addr)) = (val))

#define set_wbit(addr, v)   (*((volatile unsigned long  *)(addr)) |=  (unsigned long)(v))
#define clr_wbit(addr, v)   (*((volatile unsigned long  *)(addr)) &= ~(unsigned long)(v))

#define CONFIG_USB_1_1_DEVICE  1

#endif