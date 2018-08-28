/*
 * COM1 NS16550 support
 * originally from linux source (arch/powerpc/boot/ns16550.c)
 * modified to use CONFIG_SYS_ISA_MEM and new defines
 */

#include "usb/common.h"

#include "uart.h"
#include "gpio.h"
#include "cpu.h"
#include "ccmu.h"
#include "dma.h"

static serial_hw_t *serial_ctrl_base = NULL;
/*
 ************************************************************************************************************
 *
 *                                             function
 *
 *    name          :
 *
 *    parmeters     :
 *
 *    return        :
 *
 *    note          :
 *
 *
 ************************************************************************************************************
 */
void sunxi_serial_init(int uart_port, int bsp)
{
    u32 reg, i;
    u32 uart_clk;

    if ((uart_port < 0) || (uart_port > 0))
    {
        return;
    }
    //reset
    reg = readl(CCM_APB2_RST_REG);
    reg &= ~(1 << (CCM_UART_PORT_OFFSET + uart_port));
    writel(reg, CCM_APB2_GATE0_CTRL);
    for (i = 0; i < 100; i++)
        ;
    reg |= (1 << (CCM_UART_PORT_OFFSET + uart_port));
    writel(reg, CCM_APB2_RST_REG);
    //gate
    reg = readl(CCM_APB2_GATE0_CTRL);
    reg &= ~(1 << (CCM_UART_PORT_OFFSET + uart_port));
    writel(reg, CCM_APB2_GATE0_CTRL);
    for (i = 0; i < 100; i++)
    {
        ;
    }
    reg |= (1 << (CCM_UART_PORT_OFFSET + uart_port));
    writel(reg, CCM_APB2_GATE0_CTRL);

    //gpio
    //boot_set_gpio(gpio_cfg, gpio_max, 1);
    //uart init
    serial_ctrl_base = (serial_hw_t *) (SUNXI_UART0_BASE
            + uart_port * CCM_UART_ADDR_OFFSET);

    serial_ctrl_base->mcr = 0x3;
    uart_clk = (24000000 + 8 * bsp) / (16 * bsp);
    serial_ctrl_base->lcr |= 0x80;
    serial_ctrl_base->dlh = uart_clk >> 8;
    serial_ctrl_base->dll = uart_clk & 0xff;
    serial_ctrl_base->lcr &= ~0x80;
    serial_ctrl_base->lcr = ((PARITY & 0x03) << 3) | ((STOP & 0x01) << 2)
            | (DLEN & 0x03);
    serial_ctrl_base->fcr = 0x7;

    return;
}

static void __uart_sent_by_dma_isr(void *p_arg)
{
    int i = 0;

    for (i = 0; i < 10; i++)
    {
        ;
    }
}

void UART_DMA_R(void);

static void __uart_rev_by_dma_isr(void *p_arg)
{
    int i = 0;

    UART_DMA_R();
    for (i = 0; i < 10; i++)
    {
        ;
    }
}

static uint uart_tx_dma_hd;
static uint uart_rx_dma_hd;

int UART0_dma_init(void)
{
#define CCM_DMA_PORT_OFFSET           6

    u32 reg, i;
    uint reg_val, div;
    sunxi_dma_setting_t uart_tx_dma;
    sunxi_dma_setting_t uart_rx_dma;

    //reset   DMA时钟配置
    reg = readl(CCM_AHB1_RST_REG0);
    reg &= ~(1 << (CCM_DMA_PORT_OFFSET));
    writel(reg, CCM_AHB1_GATE0_CTRL);
    for (i = 0; i < 100; i++)
        ;
    reg |= (1 << (CCM_DMA_PORT_OFFSET));
    writel(reg, CCM_AHB1_RST_REG0);
    //gate
    reg = readl(CCM_AHB1_GATE0_CTRL);
    reg &= ~(1 << (CCM_DMA_PORT_OFFSET));
    writel(reg, CCM_AHB1_GATE0_CTRL);
    for (i = 0; i < 100; i++)
        ;
    reg |= (1 << (CCM_DMA_PORT_OFFSET));
    writel(reg, CCM_AHB1_GATE0_CTRL);

    uart_rx_dma_hd = sunxi_dma_request(DMAC_DMATYPE_NORMAL);
    uart_tx_dma_hd = sunxi_dma_request(DMAC_DMATYPE_NORMAL);

    if ((uart_tx_dma_hd == 0) || (uart_rx_dma_hd == 0))
    {
        printf("uar request dma failed\n");

        return -1;
    }
    //配置uar rx dma资源
    uart_rx_dma.loop_mode = 0;
    uart_rx_dma.wait_cyc = 1;
    uart_rx_dma.data_block_size = 1;
    //config recv(from uar fifo to dram)
    uart_rx_dma.cfg.src_drq_type = DMAC_CFG_SRC_TYPE_UART0;  //uart
    uart_rx_dma.cfg.src_addr_mode = DMAC_CFG_SRC_ADDR_TYPE_IO_MODE;
    uart_rx_dma.cfg.src_burst_length = DMAC_CFG_SRC_1_BURST;
    uart_rx_dma.cfg.src_data_width = DMAC_CFG_SRC_DATA_WIDTH_8BIT;

    uart_rx_dma.cfg.dst_drq_type = DMAC_CFG_DEST_TYPE_DRAM; //DRAM
    uart_rx_dma.cfg.dst_addr_mode = DMAC_CFG_DEST_ADDR_TYPE_LINEAR_MODE;
    uart_rx_dma.cfg.dst_burst_length = DMAC_CFG_DEST_1_BURST;
    uart_rx_dma.cfg.dst_data_width = DMAC_CFG_SRC_DATA_WIDTH_8BIT;
    uart_rx_dma.cfg.reserved0 = 0;
    uart_rx_dma.cfg.reserved1 = 0;
    uart_rx_dma.cfg.reserved2 = 0;
    uart_rx_dma.cfg.reserved3 = 0;

    //配置uar tx dma资源
    uart_tx_dma.loop_mode = 0;
    uart_tx_dma.wait_cyc = 1;
    uart_tx_dma.data_block_size = 1;
    //config send(from dram to uart fifo)
    uart_tx_dma.cfg.src_drq_type = DMAC_CFG_DEST_TYPE_DRAM;  //
    uart_tx_dma.cfg.src_addr_mode = DMAC_CFG_SRC_ADDR_TYPE_LINEAR_MODE;
    uart_tx_dma.cfg.src_burst_length = DMAC_CFG_SRC_1_BURST;
    uart_tx_dma.cfg.src_data_width = DMAC_CFG_SRC_DATA_WIDTH_8BIT;

    uart_tx_dma.cfg.dst_drq_type = DMAC_CFG_DEST_TYPE_UART0;  //UART0
    uart_tx_dma.cfg.dst_addr_mode = DMAC_CFG_DEST_ADDR_TYPE_IO_MODE;
    uart_tx_dma.cfg.dst_burst_length = DMAC_CFG_DEST_1_BURST;
    uart_tx_dma.cfg.dst_data_width = DMAC_CFG_SRC_DATA_WIDTH_8BIT;
    uart_tx_dma.cfg.reserved0 = 0;
    uart_tx_dma.cfg.reserved1 = 0;
    uart_tx_dma.cfg.reserved2 = 0;
    uart_tx_dma.cfg.reserved3 = 0;

    sunxi_dma_setting(uart_rx_dma_hd, (void *) &uart_rx_dma);
    sunxi_dma_setting(uart_tx_dma_hd, (void *) &uart_tx_dma);

    sunxi_dma_install_int(uart_tx_dma_hd, __uart_sent_by_dma_isr, 0);
    sunxi_dma_enable_int(uart_tx_dma_hd);

    sunxi_dma_install_int(uart_rx_dma_hd, __uart_rev_by_dma_isr, 0);
    sunxi_dma_enable_int(uart_rx_dma_hd);

    return 0;
}

/*
 ************************************************************************************************************
 *
 *                                             function
 *
 *    name          :
 *
 *    parmeters     :
 *
 *    return        :
 *
 *    note          :
 *
 *
 ************************************************************************************************************
 */
void sunxi_serial_putc(char c)
{
    while ((serial_ctrl_base->lsr & (1 << 6)) == 0)
        ;
    serial_ctrl_base->thr = c;
}
/*
 ************************************************************************************************************
 *
 *                                             function
 *
 *    name          :
 *
 *    parmeters     :
 *
 *    return        :
 *
 *    note          :
 *
 *
 ************************************************************************************************************
 */
char sunxi_serial_getc(void)
{
    while ((serial_ctrl_base->lsr & 1) == 0)
        ;
    return serial_ctrl_base->rbr;

}
/*
 ************************************************************************************************************
 *
 *                                             function
 *
 *    name          :
 *
 *    parmeters     :
 *
 *    return        :
 *
 *    note          :
 *
 *
 ************************************************************************************************************
 */
int sunxi_serial_tstc(void)
{
    return serial_ctrl_base->lsr & 1;
}

#pragma pack(4)
u8 uart_buf[17] = { 10, 11, 12, 13, 14, 15, 16, 17, 18, 19 };
u8 uart_rev_buf[16] = { 10, 11, 12, 13, 14, 15, 16, 17, 18, 19 };
#pragma pack()

void UART_DMA_T(void)
{
    //sunxi_dma_start(uart_tx_dma_hd, (u8)uart_buf, serial_ctrl_base->thr, 10);
    sunxi_dma_start(uart_tx_dma_hd, (uint) uart_buf,
                    (uint) &serial_ctrl_base->thr, 10);
    while (sunxi_dma_querystatus(uart_tx_dma_hd))
        ;
}

void UART_DMA_R(void)
{
    //sunxi_dma_start(uart_tx_dma_hd, (u8)uart_buf, serial_ctrl_base->thr, 10);
    sunxi_dma_start(uart_rx_dma_hd, (uint) &serial_ctrl_base->rbr,
                    (uint) uart_rev_buf, 6);
    //while(sunxi_dma_querystatus(uart_tx_dma_hd));
}

void sunxi_serial_test(void)
{
    int i = 0;
    sunxi_serial_init(0, 115200);
    sunxi_dma_init();
    UART0_dma_init();

    /*
     //UART_DMA_T();

     UART_DMA_R();

     //sunxi_serial_putc(0x55);
     //sunxi_serial_init(0,115200);

     for(i=0;i<10;i++)
     {
     //sunxi_serial_putc((char)i);
     }
     */
}

