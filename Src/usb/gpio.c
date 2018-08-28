/*
 **********************************************************************************************************************
 *
 *						           the Embedded Secure Bootloader System
 *
 *
 *						       Copyright(C), 2006-2014, Allwinnertech Co., Ltd.
 *                                           All Rights Reserved
 *
 * File    :
 *
 * By      :
 *
 * Version : V2.00
 *
 * Date	  :
 *
 * Descript:
 **********************************************************************************************************************
 */
#include "common.h"
#include "spare_head.h"
#include "cpu.h"
#include "type.h"
#include "gpio.h"
#include "timer.h"
#include "intc.h"
   
volatile __no_init CPU_INIT CPU0_Config @0x01F01C40;
volatile __no_init CPU_INIT CPU1_Config @0x01F01C80;
volatile __no_init CPU_INIT CPU2_Config @0x01F01CC0;
volatile __no_init CPU_INIT CPU3_Config @0x01F01D00;
   

void cpu_init(void)
{
   CPU0_Config.REV0 = 0;
   CPU1_Config.REV0 = 0;
   CPU2_Config.REV0 = 0;
   CPU3_Config.REV0 = 0;
}


/*
 ************************************************************************************************************
 *
 *                                             normal_gpio_cfg
 *
 *    函数名称：
 *
 *    参数列表：
 *
 *
 *
 *    返回值  ：
 *
 *    说明    ：
 *
 *
 ************************************************************************************************************
 */

void GPIO_SET_FUN(__u32 port, __u32 port_num, u8 function)
{
    __u32 tmp_group_func_data, tmp = 0;
    volatile __u32 *tmp_group_func_addr;

    tmp_group_func_addr = PIO_REG_CFG(port, port_num);
    tmp_group_func_data = GPIO_REG_READ(tmp_group_func_addr);

    tmp = port_num % 4;

    tmp_group_func_data &= ~(0x07 << tmp);

    GPIO_REG_WRITE(tmp_group_func_addr, tmp_group_func_data);     //回写功能寄存器
}

void GPIO_SET_PULL(__u32 port, __u32 port_num, u8 function)
{
    __u32 tmp_group_func_data, tmp = 0;
    volatile __u32 *tmp_group_func_addr;

    tmp_group_func_addr = PIO_REG_PULL(port, port_num);
    tmp_group_func_data = GPIO_REG_READ(tmp_group_func_addr);

    tmp = port_num % 4;

    tmp_group_func_data &= ~(0x07 << tmp);

    GPIO_REG_WRITE(tmp_group_func_addr, tmp_group_func_data);     //回写功能寄存器
}

__s32 boot_set_gpio(void *user_gpio_list, __u32 group_count_max, __s32 set_gpio)
{
    normal_gpio_set_t *tmp_user_gpio_data, *gpio_list;
    __u32 first_port;                      //保存真正有效的GPIO的个数
    __u32 tmp_group_func_data;
    __u32 tmp_group_pull_data;
    __u32 tmp_group_dlevel_data;
    __u32 tmp_group_data_data;
    __u32 data_change = 0;
//	__u32			   *tmp_group_port_addr;
    volatile __u32 *tmp_group_func_addr, *tmp_group_pull_addr;
    volatile __u32 *tmp_group_dlevel_addr, *tmp_group_data_addr;
    __u32 port, port_num, port_num_func, port_num_pull;
    __u32 pre_port, pre_port_num_func;
    __u32 pre_port_num_pull;
    __s32 i, tmp_val;

    gpio_list = (normal_gpio_set_t *) user_gpio_list;

    for (first_port = 0; first_port < group_count_max; first_port++)
    {
        tmp_user_gpio_data = gpio_list + first_port;
        port = tmp_user_gpio_data->port;                         //读出端口数值
        port_num = tmp_user_gpio_data->port_num;                 //读出端口中的某一个GPIO
        if (!port)
        {
            continue;
        }
        port_num_func = (port_num >> 3);
        port_num_pull = (port_num >> 4);

        tmp_group_func_addr = PIO_REG_CFG(port, port_num_func);   //更新功能寄存器地址
        tmp_group_pull_addr = PIO_REG_PULL(port, port_num_pull);  //更新pull寄存器
        tmp_group_dlevel_addr = PIO_REG_DLEVEL(port, port_num_pull); //更新level寄存器
        tmp_group_data_addr = PIO_REG_DATA(port);                 //更新data寄存器

        tmp_group_func_data = GPIO_REG_READ(tmp_group_func_addr);
        tmp_group_pull_data = GPIO_REG_READ(tmp_group_pull_addr);
        tmp_group_dlevel_data = GPIO_REG_READ(tmp_group_dlevel_addr);
        tmp_group_data_data = GPIO_REG_READ(tmp_group_data_addr);

        pre_port = port;
        pre_port_num_func = port_num_func;
        pre_port_num_pull = port_num_pull;
        //更新功能寄存器
        tmp_val = (port_num - (port_num_func << 3)) << 2;
        tmp_group_func_data &= ~(0x07 << tmp_val);
        if (set_gpio)
        {
            tmp_group_func_data |= (tmp_user_gpio_data->mul_sel & 0x07)
                    << tmp_val;
        }
        //根据pull的值决定是否更新pull寄存器
        tmp_val = (port_num - (port_num_pull << 4)) << 1;
        if (tmp_user_gpio_data->pull > 0)
        {
            tmp_group_pull_data &= ~(0x03 << tmp_val);
            tmp_group_pull_data |= (tmp_user_gpio_data->pull & 0x03) << tmp_val;
        }
        //根据driver level的值决定是否更新driver level寄存器
        if (tmp_user_gpio_data->drv_level > 0)
        {
            tmp_group_dlevel_data &= ~(0x03 << tmp_val);
            tmp_group_dlevel_data |= (tmp_user_gpio_data->drv_level & 0x03)
                    << tmp_val;
        }
        //根据用户输入，以及功能分配决定是否更新data寄存器
        if (tmp_user_gpio_data->mul_sel == 1)
        {
            if (tmp_user_gpio_data->data > 0)
            {
                tmp_val = tmp_user_gpio_data->data & 1;
                tmp_group_data_data &= ~(1 << port_num);
                tmp_group_data_data |= tmp_val << port_num;
                data_change = 1;
            }
        }

        break;
    }
    //检查是否有数据存在
    if (first_port >= group_count_max)
    {
        return -1;
    }
    //保存用户数据
    for (i = first_port + 1; i < group_count_max; i++)
    {
        tmp_user_gpio_data = gpio_list + i;          //gpio_set依次指向用户的每个GPIO数组成员
        port = tmp_user_gpio_data->port;                //读出端口数值
        port_num = tmp_user_gpio_data->port_num;            //读出端口中的某一个GPIO
        if (!port)
        {
            break;
        }
        port_num_func = (port_num >> 3);
        port_num_pull = (port_num >> 4);

        if ((port_num_pull != pre_port_num_pull) || (port != pre_port)) //如果发现当前引脚的端口不一致，或者所在的pull寄存器不一致
        {
            GPIO_REG_WRITE(tmp_group_func_addr, tmp_group_func_data);  //回写功能寄存器
            GPIO_REG_WRITE(tmp_group_pull_addr, tmp_group_pull_data); //回写pull寄存器
            GPIO_REG_WRITE(tmp_group_dlevel_addr, tmp_group_dlevel_data); //回写driver level寄存器
            if (data_change)
            {
                data_change = 0;
                GPIO_REG_WRITE(tmp_group_data_addr, tmp_group_data_data); //回写data寄存器
            }

            tmp_group_func_addr = PIO_REG_CFG(port, port_num_func);  //更新功能寄存器地址
            tmp_group_pull_addr = PIO_REG_PULL(port, port_num_pull); //更新pull寄存器
            tmp_group_dlevel_addr = PIO_REG_DLEVEL(port, port_num_pull); //更新level寄存器
            tmp_group_data_addr = PIO_REG_DATA(port);                //更新data寄存器

            tmp_group_func_data = GPIO_REG_READ(tmp_group_func_addr);
            tmp_group_pull_data = GPIO_REG_READ(tmp_group_pull_addr);
            tmp_group_dlevel_data = GPIO_REG_READ(tmp_group_dlevel_addr);
            tmp_group_data_data = GPIO_REG_READ(tmp_group_data_addr);
        }
        else if (pre_port_num_func != port_num_func)         //如果发现当前引脚的功能寄存器不一致
        {
            GPIO_REG_WRITE(tmp_group_func_addr, tmp_group_func_data); //则只回写功能寄存器
            tmp_group_func_addr = PIO_REG_CFG(port, port_num_func);  //更新功能寄存器地址

            tmp_group_func_data = GPIO_REG_READ(tmp_group_func_addr);
        }
        //保存当前硬件寄存器数据
        pre_port_num_pull = port_num_pull;                   //设置当前GPIO成为前一个GPIO
        pre_port_num_func = port_num_func;
        pre_port = port;

        //更新功能寄存器
        tmp_val = (port_num - (port_num_func << 3)) << 2;
        if (tmp_user_gpio_data->mul_sel > 0)
        {
            tmp_group_func_data &= ~(0x07 << tmp_val);
            if (set_gpio)
            {
                tmp_group_func_data |= (tmp_user_gpio_data->mul_sel & 0x07)
                        << tmp_val;
            }
        }
        //根据pull的值决定是否更新pull寄存器
        tmp_val = (port_num - (port_num_pull << 4)) << 1;
        if (tmp_user_gpio_data->pull > 0)
        {
            tmp_group_pull_data &= ~(0x03 << tmp_val);
            tmp_group_pull_data |= (tmp_user_gpio_data->pull & 0x03) << tmp_val;
        }
        //根据driver level的值决定是否更新driver level寄存器
        if (tmp_user_gpio_data->drv_level > 0)
        {
            tmp_group_dlevel_data &= ~(0x03 << tmp_val);
            tmp_group_dlevel_data |= (tmp_user_gpio_data->drv_level & 0x03)
                    << tmp_val;
        }
        //根据用户输入，以及功能分配决定是否更新data寄存器
        if (tmp_user_gpio_data->mul_sel == 1)
        {
            if (tmp_user_gpio_data->data > 0)
            {
                tmp_val = tmp_user_gpio_data->data & 1;
                tmp_group_data_data &= ~(1 << port_num);
                tmp_group_data_data |= tmp_val << port_num;
                data_change = 1;
            }
        }
    }
    //for循环结束，如果存在还没有回写的寄存器，这里写回到硬件当中
    if (tmp_group_func_addr)                         //只要更新过寄存器地址，就可以对硬件赋值
    {                                               //那么把所有的值全部回写到硬件寄存器
        GPIO_REG_WRITE(tmp_group_func_addr, tmp_group_func_data);   //回写功能寄存器
        GPIO_REG_WRITE(tmp_group_pull_addr, tmp_group_pull_data);   //回写pull寄存器
        GPIO_REG_WRITE(tmp_group_dlevel_addr, tmp_group_dlevel_data); //回写driver level寄存器
        if (data_change)
        {
            GPIO_REG_WRITE(tmp_group_data_addr, tmp_group_data_data); //回写data寄存器
        }
    }

    return 0;
}

/*
 ************************************************************************************************************
 *
 *                                             get_dram_type_by_gpio
 *
 *    函数名称：
 *
 *    参数列表:
 *
 *    返回值  :
 *
 *    说明    :
 *				PD17: 0X01C20800 + 0x74 [6:4]: cpux freq detect
 *
 *				PD16: 0X01C20800 + 0x74 [2:0]
 *				PD15: 0X01C20800 + 0x70 [30:28]
 *				PD14: 0X01C20800 + 0x70 [26:24]
 *				PD13: 0X01C20800 + 0x70 [22:20]
 *				PD12: 0X01C20800 + 0x70 [18:16]
 *			        [PD16:PD15:PD14:PD13:PD12]: dram type detect
 *	描述	 :
 *
 ************************************************************************************************************
 */
GPIO_ST *PORTA = (GPIO_ST *) (SUNXI_PIO_BASE);
GPIO_ST *PORTC = (GPIO_ST *) (SUNXI_PIO_BASE + 0x48);
GPIO_ST *PORTD = (GPIO_ST *) (SUNXI_PIO_BASE + 0x6c);
GPIO_ST *PORTE = (GPIO_ST *) (SUNXI_PIO_BASE + 0x90);
GPIO_ST *PORTF = (GPIO_ST *) (SUNXI_PIO_BASE + 0xB4);
GPIO_ST *PORTG = (GPIO_ST *) (SUNXI_PIO_BASE + 0xD8);
GPIO_ST *PORTL = (GPIO_ST *) (SUNXI_R_PIO_BASE + 0x00);

GP_EINT *PA_EINT = (GP_EINT *) (SUNXI_PIO_BASE + 0x200);
GP_EINT *PG_EINT = (GP_EINT *) (SUNXI_PIO_BASE + 0x220);

__u32 get_dram_type_by_gpio(void)
{
  //SUNXI_PIO_BASE = 0X01C20800
    volatile __u32 value = 0;
    volatile __u32 value_temp = 0;
    //设置gpio为输入属性
    value_temp = *((volatile unsigned int *) (SUNXI_PIO_BASE + 0x70));
    value_temp &= (~(0x0f << 28));										//PD15
    value_temp &= (~(0x0f << 24));										//PD14
    value_temp &= (~(0x0f << 20));										//PD13
    value_temp &= (~(0x0f << 16));										//PD12
    *((volatile unsigned int *) (SUNXI_PIO_BASE + 0x70)) = value_temp;

    __usdelay(10);
    value_temp = *((volatile unsigned int *) (SUNXI_PIO_BASE + 0x74));
    value_temp &= (~(0x0f << 0));										//PD16
    *((volatile unsigned int *) (SUNXI_PIO_BASE + 0x74)) = value_temp;

    __usdelay(10);
    //设置gpio为上拉模式
    value_temp = *((volatile unsigned int *) (SUNXI_PIO_BASE + 0x88));
    value_temp &= (~(0x03 << 30));										//PD15
    value_temp &= (~(0x03 << 28));										//PD14
    value_temp &= (~(0x03 << 26));										//PD13
    value_temp &= (~(0x03 << 24));										//PD12
    value_temp |= ((0x01 << 30) | (0x01 << 28) | (0x01 << 26) | (0x01 << 24));
    *((volatile unsigned int *) (SUNXI_PIO_BASE + 0x88)) = value_temp;

    __usdelay(10);
    value_temp = *((volatile unsigned int *) (SUNXI_PIO_BASE + 0x8c));
    value_temp &= (~(0x0F << 0));										//PD16
    value_temp |= (0x01 << 0);
    *((volatile unsigned int *) (SUNXI_PIO_BASE + 0x8c)) = value_temp;
    __usdelay(10);

    //读取gpio数据
    value_temp = *((volatile unsigned int *) (SUNXI_PIO_BASE + 0x7c));

    __usdelay(10);
    //value = (((value_temp >> 16) & 0x01) << 4);
    //value |= (((value_temp >> 15) & 0x01) << 3);
   // value |= (((value_temp >> 14) & 0x01) << 2);
    //value |= (((value_temp >> 13) & 0x01) << 1);
    //value |= (((value_temp >> 12) & 0x01) << 0);

    PORTA->fun10 = 1;
    PORTA->PULL10 = 1;
    PORTA->D10 = 0;
    PORTA->D10 = 1;

    PORTA->fun17 = 1;
    PORTA->PULL17 = 1;
    PORTA->D17 = 0;
    PORTA->D17 = 1;

    PORTC->fun0 = 3;

    return value;
}

extern volatile u8 Reciveddata;
extern void spi_dma_rev();

void GPA_EINT_func(void *data)
{
    u32 status = PA_EINT->PX_EINT_STATUS_REG;
    if (status & (0x40))
    {
        PA_EINT->PX_EINT_STATUS_REG = status; //清除中断标记

        if(Reciveddata == 0)
        {
            spi_dma_rev();
        }
    }
}

void EINT_A6_config(void)
{
    PORTA->fun6 = 6; //外部中断模式

    PA_EINT->MODE6 = POS_EDGE;
    PA_EINT->CLK_SEL = 1; //24Mclk
    PA_EINT->CLK_DIV = 0;
    PA_EINT->PX_EINT_CTL_REG = (1 << 6);

    irq_install_handler(AW_IRQ_EINTA, GPA_EINT_func, 0);

    irq_enable(AW_IRQ_EINTA);
}

//-------------------------end of file--------------------------

