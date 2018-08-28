/*
 * (C) Copyright 2007-2013
 * MA 02111-1307 USA
 */

#ifndef _SUNXI_GPIO_H
#define _SUNXI_GPIO_H

//#include "cpu.h"

#define GPIO_REG_READ(reg)              readl((reg))
#define GPIO_REG_WRITE(reg, value)      writel((value), (reg))

#define PIOC_REG_o_CFG0                 0x00
#define PIOC_REG_o_CFG1                 0x04
#define PIOC_REG_o_CFG2                 0x08
#define PIOC_REG_o_CFG3                 0x0C
#define PIOC_REG_o_DATA                 0x10
#define PIOC_REG_o_DRV0                 0x14
#define PIOC_REG_o_DRV1                 0x18
#define PIOC_REG_o_PUL0                 0x1C
#define PIOC_REG_o_PUL1                 0x20

/**#############################################################################################################
 *
 *                           GPIO(PIN) Operations
 *
 -##############################################################################################################*/
#define PIO_REG_CFG(n, i)               ((volatile unsigned int *)( SUNXI_PIO_BASE + ((n)-1)*0x24 + ((i)<<2) + 0x00))
#define PIO_REG_DLEVEL(n, i)            ((volatile unsigned int *)( SUNXI_PIO_BASE + ((n)-1)*0x24 + ((i)<<2) + 0x14))
#define PIO_REG_PULL(n, i)              ((volatile unsigned int *)( SUNXI_PIO_BASE + ((n)-1)*0x24 + ((i)<<2) + 0x1C))
#define PIO_REG_DATA(n)                   ((volatile unsigned int *)( SUNXI_PIO_BASE + ((n)-1)*0x24 + 0x10))

#define PIO_REG_CFG_VALUE(n, i)          readl( SUNXI_PIO_BASE + ((n)-1)*0x24 + ((i)<<2) + 0x00)
#define PIO_REG_DLEVEL_VALUE(n, i)       readl( SUNXI_PIO_BASE + ((n)-1)*0x24 + ((i)<<2) + 0x14)
#define PIO_REG_PULL_VALUE(n, i)         readl( SUNXI_PIO_BASE + ((n)-1)*0x24 + ((i)<<2) + 0x1C)
#define PIO_REG_DATA_VALUE(n)            readl( SUNXI_PIO_BASE + ((n)-1)*0x24 + 0x10)
#define PIO_REG_BASE(n)                    ((volatile unsigned int *)(SUNXI_PIO_BASE +((n)-1)*24))

#ifdef SUNXI_R_PIO_BASE
#define R_PIO_REG_CFG(n, i)               ((volatile unsigned int *)( SUNXI_R_PIO_BASE + ((n)-12)*0x24 + ((i)<<2) + 0x00))
#define R_PIO_REG_DLEVEL(n, i)            ((volatile unsigned int *)( SUNXI_R_PIO_BASE + ((n)-12)*0x24 + ((i)<<2) + 0x14))
#define R_PIO_REG_PULL(n, i)              ((volatile unsigned int *)( SUNXI_R_PIO_BASE + ((n)-12)*0x24 + ((i)<<2) + 0x1C))
#define R_PIO_REG_DATA(n)                 ((volatile unsigned int *)( SUNXI_R_PIO_BASE + ((n)-12)*0x24 + 0x10))

#define R_PIO_REG_CFG_VALUE(n, i)          readl( SUNXI_R_PIO_BASE + ((n)-12)*0x24 + ((i)<<2) + 0x00)
#define R_PIO_REG_DLEVEL_VALUE(n, i)       readl( SUNXI_R_PIO_BASE + ((n)-12)*0x24 + ((i)<<2) + 0x14)
#define R_PIO_REG_PULL_VALUE(n, i)         readl( SUNXI_R_PIO_BASE + ((n)-12)*0x24 + ((i)<<2) + 0x1C)
#define R_PIO_REG_DATA_VALUE(n)            readl( SUNXI_R_PIO_BASE + ((n)-12)*0x24 + 0x10)
#define R_PIO_REG_BASE(n)                    ((volatile unsigned int *)(SUNXI_R_PIO_BASE +((n)-12)*24))
#endif
typedef struct
{
    int mul_sel;
    int pull;
    int drv_level;
    int data;
} gpio_status_set_t;

typedef struct
{
    char gpio_name[32];
    int port;
    int port_num;
    gpio_status_set_t user_gpio_status;
    gpio_status_set_t hardware_gpio_status;
} system_gpio_set_t;

//通用的，和GPIO相关的数据结构
typedef struct
{
    unsigned char port;                       //端口号
    unsigned char port_num;                   //端口内编号
    char mul_sel;                    //功能编号
    char pull;                       //电阻状态
    char drv_level;                  //驱动驱动能力
    char data;                       //输出电平
    unsigned char reserved[2];                //保留位，保证对齐
} normal_gpio_set_t;

int boot_set_gpio(void *user_gpio_list, __u32 group_count_max, __s32 set_gpio);
__u32 get_dram_type_by_gpio(void);

#define GPIO_ACTIVE_HIGH 0
#define GPIO_ACTIVE_LOW 1
//------------------------------------------------------------


volatile __no_init typedef struct
{
    //__IO u32 RG[4];
    union
    {
        __IO u32 RG0;
        struct
        {
            __IO u32 fun0 :4;
            __IO u32 fun1 :4;
            __IO u32 fun2 :4;
            __IO u32 fun3 :4;
            __IO u32 fun4 :4;
            __IO u32 fun5 :4;
            __IO u32 fun6 :4;
            __IO u32 fun7 :4;
        };
    };
    union
    {
        __IO u32 RG1;
        struct
        {
            __IO u32 fun8 :4;
            __IO u32 fun9 :4;
            __IO u32 fun10 :4;
            __IO u32 fun11 :4;
            __IO u32 fun12 :4;
            __IO u32 fun13 :4;
            __IO u32 fun14 :4;
            __IO u32 fun15 :4;
        };
    };
    union
    {
        __IO u32 RG2;
        struct
        {
            __IO u32 fun16 :4;
            __IO u32 fun17 :4;
            __IO u32 fun18 :4;
            __IO u32 fun19 :4;
            __IO u32 fun20 :4;
            __IO u32 fun21 :4;
            __IO u32 fun22 :4;
            __IO u32 fun23 :4;
        };
    };
    union
    {
        __IO u32 RG3;
        struct
        {
            __IO u32 fun24 :4;
            __IO u32 fun25 :4;
            __IO u32 fun26 :4;
            __IO u32 fun27 :4;
            __IO u32 fun28 :4;
            __IO u32 fun29 :4;
            __IO u32 fun30 :4;
            __IO u32 fun31 :4;
        };
    };

    //__IO u32 DATA;
    union
    {
        __IO u32 DATA;
        struct
        {
            __IO u32 D0 :1;
            __IO u32 D1 :1;
            __IO u32 D2 :1;
            __IO u32 D3 :1;
            __IO u32 D4 :1;
            __IO u32 D5 :1;
            __IO u32 D6 :1;
            __IO u32 D7 :1;
            __IO u32 D8 :1;
            __IO u32 D9 :1;
            __IO u32 D10 :1;
            __IO u32 D11 :1;
            __IO u32 D12 :1;
            __IO u32 D13 :1;
            __IO u32 D14 :1;
            __IO u32 D15 :1;
            __IO u32 D16 :1;
            __IO u32 D17 :1;
            __IO u32 D18 :1;
            __IO u32 D19 :1;
            __IO u32 D20 :1;
            __IO u32 D21 :1; 
            __IO u32 D22_31 :10;
        };
    };

    //__IO u32 MD_REG[2];
    union
    {
        __IO u32 MD_REG0;
        struct
        {
            __IO u32 DVR0 :2;
            __IO u32 DVR1 :2;
            __IO u32 DVR2 :2;
            __IO u32 DVR3 :2;
            __IO u32 DVR4 :2;
            __IO u32 DVR5 :2;
            __IO u32 DVR6 :2;
            __IO u32 DVR7 :2;
            __IO u32 DVR8 :2;
            __IO u32 DVR9 :2;
            __IO u32 DVR10 :2;
            __IO u32 DVR11 :2;
            __IO u32 DVR12 :2;
            __IO u32 DVR13 :2;
            __IO u32 DVR14 :2;
            __IO u32 DVR15 :2;
        };
    };
    union
    {
        __IO u32 MD_REG1;
        struct
        {
            __IO u32 DVR16 :2;
            __IO u32 DVR17 :2;
            __IO u32 DVR18 :2;
            __IO u32 DVR19 :2;
            __IO u32 DVR20 :2;
            __IO u32 DVR21 :2;
            __IO u32 DVR22 :2;
            __IO u32 DVR23 :2;
            __IO u32 DVR24 :2;
            __IO u32 DVR25 :2;
            __IO u32 DVR26 :2;
            __IO u32 DVR27 :2;
            __IO u32 DVR28 :2;
            __IO u32 DVR29 :2;
            __IO u32 DVR30 :2;
            __IO u32 DVR31 :2;
        };
    };
    //__IO u32 PULL[2];
    union
    {
        __IO u32 PULL_REG0;
        struct
        {
            __IO u32 PULL0 :2;
            __IO u32 PULL1 :2;
            __IO u32 PULL2 :2;
            __IO u32 PULL3 :2;
            __IO u32 PULL4 :2;
            __IO u32 PULL5 :2;
            __IO u32 PULL6 :2;
            __IO u32 PULL7 :2;
            __IO u32 PULL8 :2;
            __IO u32 PULL9 :2;
            __IO u32 PULL10 :2;
            __IO u32 PULL11 :2;
            __IO u32 PULL12 :2;
            __IO u32 PULL13 :2;
            __IO u32 PULL14 :2;
            __IO u32 PULL15 :2;
        };
    };
    union
    {
        __IO u32 PULL_REG1;
        struct
        {
            __IO u32 PULL16 :2;
            __IO u32 PULL17 :2;
            __IO u32 PULL18 :2;
            __IO u32 PULL19 :2;
            __IO u32 PULL20 :2;
            __IO u32 PULL21 :2;
            __IO u32 PULL22 :2;
            __IO u32 PULL23 :2;
            __IO u32 PULL24 :2;
            __IO u32 PULL25 :2;
            __IO u32 PULL26 :2;
            __IO u32 PULL27 :2;
            __IO u32 PULL28 :2;
            __IO u32 PULL29 :2;
            __IO u32 PULL30 :2;
            __IO u32 PULL31 :2;
        };
    };

} GPIO_ST;

extern GPIO_ST *PORTA;
extern GPIO_ST *PORTC;
extern GPIO_ST *PORTD;
extern GPIO_ST *PORTE;
extern GPIO_ST *PORTF;
extern GPIO_ST *PORTG;

extern GPIO_ST *PORTL;

//----EXIT INtrttruput A
volatile __no_init typedef struct
{
    //__IO u32 RG[4];
    union
    {
        __IO u32 PX_EINT_CFG0;
        struct
        {
            __IO u32 MODE0 :4;
            __IO u32 MODE1 :4;
            __IO u32 MODE2 :4;
            __IO u32 MODE3 :4;
            __IO u32 MODE4 :4;
            __IO u32 MODE5 :4;
            __IO u32 MODE6 :4;
            __IO u32 MODE7 :4;
        };
    };
    union
    {
        __IO u32 PX_EINT_CFG1;
        struct
        {
            __IO u32 MODE8 :4;
            __IO u32 MODE9 :4;
            __IO u32 MODE10 :4;
            __IO u32 MODE11 :4;
            __IO u32 MODE12 :4;
            __IO u32 MODE13 :4;
            __IO u32 MODE14 :4;
            __IO u32 MODE15 :4;
        };
    };
    union
    {
        __IO u32 PX_EINT_CFG2;
        struct
        {
            __IO u32 MODE16 :4;
            __IO u32 MODE17 :4;
            __IO u32 MODE18 :4;
            __IO u32 MODE19 :4;
            __IO u32 MODE20 :4;
            __IO u32 MODE21 :4;
            __IO u32 MODE22 :4;
            __IO u32 MODE23 :4;
        };
    };
    union
    {
        __IO u32 PX_EINT_CFG3;
        struct
        {
            __IO u32 MODE24 :4;
            __IO u32 MODE25 :4;
            __IO u32 MODE26 :4;
            __IO u32 MODE27 :4;
            __IO u32 MODE28 :4;
            __IO u32 MODE29 :4;
            __IO u32 MODE30 :4;
            __IO u32 MODE31 :4;
        };
    };
    //-----
    __IO u32 PX_EINT_CTL_REG;
    __IO u32 PX_EINT_STATUS_REG;
    union
    {
        __IO u32 PX_EINT_DEB_REG;
        struct
        {
            __IO u32 CLK_SEL :1;
            __IO u32 RESERV0 :3;
            __IO u32 CLK_DIV :3;
            __IO u32 RESERV1 :25;
        };
    };
} GP_EINT;

extern GP_EINT *PA_EINT;
extern GP_EINT *PG_EINT;

#define  POS_EDGE   0x0
#define  NEG_EDGE   0x1
#define  HIGH_LEV   0x2
#define  LOW_LEV    0x3
#define  DOUBLE_EDG 0x4

extern void EINT_A6_config(void);



volatile __no_init typedef struct
{ 
    union
    {
        __IO u32 CPU_RST_CTRL;
        struct
        {
            __IO u32 CPU_REST :1;
            __IO u32 CORE_RST :1;
            __IO u32 REV0 :30; 
        };
    };
    
    union
    {
        __IO u32 CPU_CTRL_REG;
        struct
        {
            __IO u32 CPU_CP15_ACC:1; 
            __IO u32 REV1 :31; 
        };
    };
    
    union
    {
        __IO u32 CPU_STATUS_REG;
        struct
        {
            __IO u32 AMP_SMP :1;
            __IO u32 WFE :1;
            __IO u32 WFI :1; 
            __IO u32 REV2 :29; 
        };
    };
    
    
}CPU_INIT;





#endif /* _SUNXI_GPIO_H */
