/*
 * (C) Copyright 2007-2013
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * Jerry Wang <wangflord@allwinnertech.com>
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include "type.h"
#include "spi_hal.h"
#include "dma.h"
#include "stdio.h"

static u32 g_cfg_mclk = 0;
static uint spi_tx_dma_hd;
static uint spi_rx_dma_hd;
#if 1
#define SUNXI_DEBUG(fmt,args...)	printf(fmt ,##args)
#else
#define SUNXI_DEBUG(fmt,args...) do {} while(0)
#endif

void spic_config_dual_mode(u32 spi_no, u32 rxdual, u32 dbc, u32 stc)
{
    writel((rxdual << 28) | (dbc << 24) | (stc), SPI_BCC);
}

void ccm_module_disable_bak(u32 clk_id)
{
    switch (clk_id >> 8)
    {
    case AHB1_BUS0:
        clr_wbit(CCM_AHB1_RST_REG0, 0x1U << (clk_id & 0xff));
        SUNXI_DEBUG("\nread CCM_AHB1_RST_REG0[0x%x]\n",
                    readl(CCM_AHB1_RST_REG0));
        break;
    }
}

void ccm_module_enable_bak(u32 clk_id)
{
    switch (clk_id >> 8)
    {
    case AHB1_BUS0:
        set_wbit(CCM_AHB1_RST_REG0, 0x1U << (clk_id & 0xff));
        SUNXI_DEBUG("\nread enable CCM_AHB1_RST_REG0[0x%x]\n",
                    readl(CCM_AHB1_RST_REG0));
        break;
    }
}
void ccm_clock_enable_bak(u32 clk_id)
{
    switch (clk_id >> 8)
    {
    case AXI_BUS:
        set_wbit(CCM_AXI_GATE_CTRL, 0x1U << (clk_id & 0xff));
        break;
    case AHB1_BUS0:
        set_wbit(CCM_AHB1_GATE0_CTRL, 0x1U << (clk_id & 0xff));
        SUNXI_DEBUG("read s CCM_AHB1_GATE0_CTRL[0x%x]\n",
                    readl(CCM_AHB1_GATE0_CTRL));
        break;
    }
}

void ccm_clock_disable_bak(u32 clk_id)
{
    switch (clk_id >> 8)
    {
    case AXI_BUS:
        clr_wbit(CCM_AXI_GATE_CTRL, 0x1U << (clk_id & 0xff));
        break;
    case AHB1_BUS0:
        clr_wbit(CCM_AHB1_GATE0_CTRL, 0x1U << (clk_id & 0xff));
        SUNXI_DEBUG("read dis CCM_AHB1_GATE0_CTRL[0x%x]\n",
                    readl(CCM_AHB1_GATE0_CTRL));
        break;
    }
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

#define set_wbit(addr, v)   (*((volatile unsigned long  *)(addr)) |=  (unsigned long)(v))
#define clr_wbit(addr, v)   (*((volatile unsigned long  *)(addr)) &= ~(unsigned long)(v))

void ccm_module_reset_bak(u32 clk_id)
{
    ccm_module_disable_bak(clk_id);
    ccm_module_disable_bak(clk_id);
    ccm_module_enable_bak(clk_id);
}

u32 ccm_get_pll_periph_clk(void)
{
    u32 rval = 0;
    u32 n, k;
    rval = readl(CCM_PLL6_MOD_CTRL);
    n = (0x1f & (rval >> 8)) + 1;
    k = (0x3 & (rval >> 4)) + 1;
    return (24000000 * n * k) >> 1;
}

void pattern_goto(int pos)
{
    //SUNXI_DEBUG("pos =%d\n",pos);
}

u32 spi_cfg_mclk(u32 spi_no, u32 src, u32 mclk)
{
#ifdef CONFIG_FPGA
    g_cfg_mclk = 24000000;
    return g_cfg_mclk;
#else
    u32 mclk_base = CCM_SPI0_SCLK_CTRL;
    u32 source_clk;
    u32 rval;
    u32 m, n, div;
    //src  = 1;

    switch (src)
    {
    case 0:
        source_clk = 24000000;
        break;
    case 1:
        source_clk = ccm_get_pll_periph_clk();
        break;
    default:
        SUNXI_DEBUG("Wrong SPI clock source :%x\n", src);
    }
    SUNXI_DEBUG("SPI clock source :%d\n", source_clk);
    div = (source_clk + mclk - 1) / mclk;
    div = div == 0 ? 1 : div;
    if (div > 128)
    {
        m = 1;
        n = 0;
        SUNXI_DEBUG("Source clock is too high\n");
    }
    else if (div > 64)
    {
        n = 3;
        m = div >> 3;
    }
    else if (div > 32)
    {
        n = 2;
        m = div >> 2;
    }
    else if (div > 16)
    {
        n = 1;
        m = div >> 1;
    }
    else
    {
        n = 0;
        m = div;
    }

    rval = (1U << 31) | (src << 24) | (n << 16) | (m - 1);
    writel(rval, mclk_base);
    g_cfg_mclk = source_clk / (1 << n) / (m - 1);
    SUNXI_DEBUG("spi spic->sclk =%d\n", g_cfg_mclk);
    return g_cfg_mclk;
#endif
}

u32 spi_get_mlk(u32 spi_no)
{
    return g_cfg_mclk; //spicinfo[spi_no].sclk;
}

void spi_gpio_cfg(int spi_no)
{
    uint reg_val = 0;
    writel(0x3333, (0x1c20800 + 0x48));   // PIO SETTING,PortC SPI0
    reg_val = readl(0x1c20800 + 0x64);	  // PIO SETTING,PortC SPI0 pull reg
    reg_val &= ~(0x03 << 4);
    reg_val |= (0x01 << 4);
    writel(reg_val, (0x1c20800 + 0x64));

    SUNXI_DEBUG("Reg pull reg_val=0x%x,read=0x%x\n", reg_val,
                readl((0x1c20800 + 0x64)));
}

void spi_onoff(u32 spi_no, u32 onoff)
{
    u32 clkid[] = { SPI0_CKID, SPI1_CKID };
    //u32 reg_val = 0;
    spi_no = 0;
    switch (spi_no)
    {
    case 0:
        spi_gpio_cfg(0);
        break;
    }
    ccm_module_reset_bak(clkid[spi_no]);
    if (onoff)
        ccm_clock_enable_bak(clkid[spi_no]);
    else
        ccm_clock_disable_bak(clkid[spi_no]);

}

void spic_set_clk(u32 spi_no, u32 clk)
{
    u32 mclk = spi_get_mlk(spi_no);
    u32 div;
    u32 cdr1 = 0;
    u32 cdr2 = 0;
    u32 cdr_sel = 0;

    div = mclk / (clk << 1);

    if (div == 0)
    {
        cdr1 = 0;

        cdr2 = 0;
        cdr_sel = 0;
    }
    else if (div <= 0x100)
    {
        cdr1 = 0;

        cdr2 = div - 1;
        cdr_sel = 1;
    }
    else
    {
        div = 0;
        while (mclk > clk)
        {
            div++;
            mclk >>= 1;
        }
        cdr1 = div;

        cdr2 = 0;
        cdr_sel = 0;
    }

    writel((cdr_sel << 12) | (cdr1 << 8) | cdr2, SPI_CCR);
    SUNXI_DEBUG("spic_set_clk:mclk=%d\n", mclk);
}

void spi_rev_func(void *data)
{
    int i = 0;

    u32 stat = readl(SPI_ISR);

    if (stat & 0x01)
    {
        writel(stat, SPI_ISR);
    }
    /*
     for(i=0;i<10;i++)
     {
     ;
     }
     */
}

SPI_REG *SPI0_ctl = (SPI_REG *) (SPI_BASE);

int spic_init(u32 spi_no)
{
    u32 rval = 0;
    //uint reg_val, div;
    spi_no = 0;
    spi_onoff(spi_no, 1);

	SPI0_ctl->SPI_GCR_WR = 0x10000000; //rest all spi

	spi_cfg_mclk(spi_no, SPI_CLK_SRC, SPI_MCLK);
	spic_set_clk(spi_no, SPI_DEFAULT_CLK);

    //rval = SPI_SOFT_RST|SPI_TXPAUSE_EN|SPI_MASTER|SPI_ENABLE;   //复位，接收fifo满停止传输，主机模式，SPI使能
    rval = SPI_SOFT_RST | SPI_TXPAUSE_EN | SPI_ENABLE;
    writel(rval, SPI_GCR);

    rval = SPI_SET_SS_1 | SPI_DHB | SPI_SS_ACTIVE0|SPI_MODE0; //set ss to high,discard unused burst,SPI select signal polarity(low,1=idle)
    writel(rval, SPI_TCR);

    writel(SPI_RXFIFO_RST|SPI_TXFIFO_RST|(SPI_RX_WL)|SPI_RXDMAREQ_EN|(1<<9) , SPI_FCR);

   // writel(((0 << 13) | 0x00), SPI_IER); //接收中断使能

    //irq_install_handler(AW_IRQ_SPI0, spi_rev_func, 0);

    //AW_IRQ_SPI0
    //irq_enable(AW_IRQ_SPI0);

    return 0;
}

void spi_dma_rev();
volatile u8 Reciveddata = 0;
volatile u8 spi_buf[3879] = { 0 };

static void __spi_rev_by_dma_isr(void *p_arg)
{
    int i = 0;

    Reciveddata = 1;

    spi_dma_rev();
}

int spic_init_dma(uint spi_no)
{
    uint reg_val, div;

#define CCM_DMA_PORT_OFFSET           6

    u32 reg, i;

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

    sunxi_dma_setting_t spi_tx_dma;
    sunxi_dma_setting_t spi_rx_dma;

    spi_rx_dma_hd = sunxi_dma_request(DMAC_DMATYPE_NORMAL);
    spi_tx_dma_hd = sunxi_dma_request(DMAC_DMATYPE_NORMAL);

    if ((spi_tx_dma_hd == 0) || (spi_rx_dma_hd == 0))
    {
        printf("spi request dma failed\n");

        return -1;
    }

    spi_rx_dma.cfg.src_drq_type = DMAC_CFG_DEST_TYPE_SPI0;  //SPI0
    spi_rx_dma.cfg.src_addr_mode = DMAC_CFG_SRC_ADDR_TYPE_IO_MODE;
    spi_rx_dma.cfg.src_burst_length = DMAC_CFG_SRC_1_BURST;
    spi_rx_dma.cfg.src_data_width = DMAC_CFG_SRC_DATA_WIDTH_8BIT;
    spi_rx_dma.cfg.reserved0 = 0;
    spi_rx_dma.cfg.reserved1 = 0;
    spi_rx_dma.cfg.reserved2 = 0;
    spi_rx_dma.cfg.reserved3 = 0;
    spi_rx_dma.data_block_size = 3872;
    spi_rx_dma.loop_mode = 1;
    spi_rx_dma.wait_cyc = 1;

    spi_rx_dma.cfg.dst_drq_type = DMAC_CFG_DEST_TYPE_DRAM;  //DRAM
    spi_rx_dma.cfg.dst_addr_mode = DMAC_CFG_DEST_ADDR_TYPE_LINEAR_MODE;
    spi_rx_dma.cfg.dst_burst_length = DMAC_CFG_DEST_1_BURST;
    spi_rx_dma.cfg.dst_data_width = DMAC_CFG_DEST_DATA_WIDTH_8BIT;

        /*
	spi_tx_dma.cfg.src_drq_type     = DMAC_CFG_DEST_TYPE_DRAM;  //
	spi_tx_dma.cfg.src_addr_mode    = DMAC_CFG_SRC_ADDR_TYPE_LINEAR_MODE;
	spi_tx_dma.cfg.src_burst_length = DMAC_CFG_SRC_1_BURST;
	spi_tx_dma.cfg.src_data_width   = DMAC_CFG_SRC_DATA_WIDTH_8BIT;

	spi_tx_dma.cfg.dst_drq_type     = DMAC_CFG_DEST_TYPE_SPI0;  //SPI0
	spi_tx_dma.cfg.dst_addr_mode    = DMAC_CFG_DEST_ADDR_TYPE_IO_MODE;
	spi_tx_dma.cfg.dst_burst_length = DMAC_CFG_DEST_1_BURST;
	spi_tx_dma.cfg.dst_data_width   = DMAC_CFG_DEST_DATA_WIDTH_8BIT;
          */
	sunxi_dma_setting(spi_rx_dma_hd, (void *)&spi_rx_dma);
	//sunxi_dma_setting(spi_tx_dma_hd, (void *)&spi_tx_dma);

    sunxi_dma_install_int(spi_rx_dma_hd, __spi_rev_by_dma_isr, 0);
    sunxi_dma_enable_int(spi_rx_dma_hd);

    /*
     spi_set_src_clk(0, 2, SPI_MCLK);

     spi_onoff(spi_no, 0);
     spi_onoff(spi_no, 1);

     reg_val = (1 << SPI_CTL_TPE_OFFSET) | (1 << SPI_CTL_SS_LEVEL_OFFSET)
     | (1 << SPI_CTL_DHB_OFFSET) | (1 << SPI_CTL_SMC_OFFSET) | (1 << SPI_CTL_RFRST_OFFSET)
     | (1 << SPI_CTL_TFRST_OFFSET) | (1 << SPI_CTL_SSPOL_OFFSET) | (1 << SPI_CTL_POL_OFFSET) | (1 << SPI_CTL_PHA_OFFSET) | (1 << SPI_CTL_MODE_OFFSET) | (1 << SPI_CTL_EN_OFFSET);

     writel(reg_val, SPI_CONTROL);
     writel(0, SPI_DMACTRL);

     // set spi clock
     printf("ahb clock=%d\n", div = sunxi_clock_get_ahb());
     //div = sunxi_clock_get_ahb() / 20 - 1;
     div = div/20-1;
     reg_val  = 1 << 12;
     reg_val |= div;
     writel(reg_val, SPI_CLCKRATE);
     writel(0, SPI_WATCLCK);
     //Clear Status Register
     writel(0xfffff, SPI_ISR);
     //wait clear busy, add in aw1623, 2013-12-22 11:25:03
     while (readl(SPI_ISR) & (1U << 31));
     if(readl(SPI_ISR) != 0x1b00)
     {
     printf("SPI Status Register Initialized Fail: \n", readl(SPI_ISR));

     return -1;
     }

     //Set Interrput Control Register
     writel(0x0000, SPI_INTCTRL);        //Close All Interrupt
     */
    return 0;
}

int spic_rw(u32 tcnt, void* txbuf, u32 rcnt, void* rxbuf)
{
    u32 i = 0, fcr;
    int timeout = 0xfffff;
    //uint ret = 0;

    u8 *tx_buffer = txbuf;
    u8 *rx_buffer = rxbuf;
    writel(0, SPI_IER);
    writel(0xffffffff, SPI_ISR);  //clear status register

    writel(tcnt, SPI_MTC);
    writel(tcnt + rcnt, SPI_MBC);
    writel(readl(SPI_TCR)|SPI_EXCHANGE, SPI_TCR);
    if (tcnt)
    {
        //if(tcnt < 1024)
        {
            i = 0;
            while (i < tcnt)
            {
                //send data
                while (((readl(SPI_FSR) >> 16) == SPI_FIFO_SIZE))
                    ;
                writeb(*(tx_buffer + i), SPI_TXD);
                i++;
            }
        }
        //else
        //	{
        //spi_no = 0;
        //		writel((readl(SPI_FCR)|SPI_TXDMAREQ_EN), SPI_FCR);
        //		spi_dma_send_start(0, txbuf, tcnt);
        /* wait DMA finish */
        //		while ((timeout-- > 0) && spi_wait_dma_send_over(0));
        //		if (timeout <= 0)
        //		{
        //			printf("tx wait_dma_send_over fail\n");
        //			return -1;
        //		}
        //	printf("timeout %d,count_send %d \n",timeout,count_send);
        //	}
    }
    timeout = 0xfffff;
    /* start transmit */
    if (rcnt)
    {
        //if(rcnt < 1024)
        {
            i = 0;
            while (i < rcnt)
            {
                //receive valid data
                while (((readl(SPI_FSR)) & 0x7f) == 0)
                    ;
                *(rx_buffer + i) = readb(SPI_RXD);
                i++;
            }
        }
        //else
        //	{
        //       pattern_goto(115);
        //		timeout = 0xfffff;
        //		writel((readl(SPI_FCR)|SPI_RXDMAREQ_EN), SPI_FCR);
        //		spi_dma_recv_start(0, rxbuf, rcnt);
        /* wait DMA finish */
        //		while ((timeout-- > 0) && spi_wait_dma_recv_over(0));
        //		if (timeout <= 0)
        //		{
        //			printf("rx wait_dma_recv_over fail\n");
        //			return -1;
        //		}
        //	}
    }

    fcr = readl(SPI_FCR);
    fcr &= ~(SPI_TXDMAREQ_EN | SPI_RXDMAREQ_EN);
    writel(fcr, SPI_FCR);
    if ((readl(SPI_ISR) & (0xf << 8)) || (timeout == 0)) /* (1U << 11) | (1U << 10) | (1U << 9) | (1U << 8)) */
        return RET_FAIL;

    if (readl(SPI_TCR) & SPI_EXCHANGE)
    {
        printf("XCH Control Error!!\n");
    }

    writel(0xfffff, SPI_ISR); /* clear  flag */
    return RET_OK;

}

static int spi_dma_recv_start(uint spi_no, uchar* pbuf, uint byte_cnt)
{
    flush_cache((uint) pbuf, byte_cnt);

    sunxi_dma_start(spi_rx_dma_hd, SPI_RXD, (uint) pbuf, byte_cnt);

    return 0;
}



void spi_dma_rev()
{
    spi_dma_recv_start(0, ( uchar* )&spi_buf[4], 3872);
}

int spic_rw_dma_rev(uint scount, void *saddr, uint rcount, void *raddr)
{
    int time, ret = -1;
    u8 *tx_addr, *rx_addr;
    int rdma;

    if ((scount + rcount) > 64 * 1024)
    {
        printf("too much data to transfer at once\n");

        return -1;
    }

    tx_addr = (u8 *) saddr;
    rx_addr = (u8 *) raddr;

    writel(readl(SPI_ISR) | 0xffff, SPI_ISR);
    //writel(0, SPI_DMACTRL);
    writel(scount + rcount, SPI_MBC);
    writel(scount, SPI_MTC);

    if (rcount > 7)
    {
        /* RXFIFO half full dma request enable */
        // writel(0x02, SPI_DMACTRL);
        flush_cache((uint) raddr, rcount);
        spi_dma_recv_start(0, raddr, rcount);

        rdma = 1;
        rcount = 0;
    }
    writel(readl(SPI_GCR) | (1 << 0x0a), SPI_GCR);

    if (scount)
    {
        time = 0xffffff;
        if (scount > 7)
        {
            /* RXFIFO half empty dma request enable */
            // writel(readl(SPI_DMACTRL) | 0x0200, SPI_DMACTRL);
            spi_dma_send_start(0, saddr, scount);
            /* wait DMA finish */
            while ((time-- > 0) && spi_wait_dma_send_over(0))
                ;
        }
        else
        {
            for (; scount > 0; scount--)
            {
                *(volatile u8 *) (SPI_TXD) = *tx_addr;
                tx_addr += 1;
            }

            time = 0xffffff;
            while ((readl(SPI_ISR) >> 20) & 0x0f)
            {
                time--;
                if (time <= 0)
                {
                    printf("LINE: %d\n", __LINE__);

                    return ret;
                }
            }
        }

        if (time <= 0)
        {
            printf("LINE: %d\n", __LINE__);

            return ret;
        }
    }

    time = 0xffff;
    while (rcount && (time > 0))
    {
        //spi_test_counter = 0;
        if ((readl(SPI_ISR) >> 16) & 0x0f)
        {
            *rx_addr++ = *(volatile u8 *) (SPI_RXD);
//            spi_test_counter ++;
//            if(spi_test_counter = 7)
//            {
//            	puts("\n");
//            }
//            printf("0x%02x  ", *(rx_addr-1));
            --rcount;
            time = 0xffff;
        }
        --time;
    }

    if (time <= 0)
    {
        printf("LINE: %d\n", __LINE__);

        return ret;
    }

    if (rdma)
    {
        time = 0xffffff;
        while ((time-- > 0) && spi_wait_dma_recv_over(0))
            ;
        if (time <= 0)
        {
            printf("LINE: %d\n", __LINE__);

            return ret;
        }
    }

    if (time > 0)
    {
        uint tmp;

        time = 0xfffff;

        tmp = (readl(SPI_ISR) >> 16) & 0x01;

        do
        {
            tmp = (readl(SPI_ISR) >> 16) & 0x01;
            if ((time--) <= 0)
            {
                printf("LINE: %d\n", __LINE__);

                return ret;
            }
        }
        while (!tmp);
    }

    return 0;
}

int spic_exit(u32 spi_no)
{
    return 0;
}

void spi_test(void)
{
    spi_buf[0] = 0x55;
    spi_buf[1] = 0xaa;
    spic_init(0);
    sunxi_dma_init();
    spic_init_dma(0);
    spi_dma_rev();
}

//----------------------
