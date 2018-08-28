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
#ifndef   __SPI_H__
#define   __SPI_H__

#include  "cpu.h"
#include  "type.h"

#define SPI_CTL_MSDC_OFFSET     0x13
#define SPI_CTL_TPE_OFFSET      0x12
#define SPI_CTL_SS_LEVEL_OFFSET 0x11
#define SPI_CTL_SS_CTL_OFFSET   0x10
#define SPI_CTL_DHB_OFFSET      0x0f
#define SPI_CTL_DDB_OFFSET      0x0e
#define SPI_CTL_SS_OFFSET       0x0c
#define SPI_CTL_SMC_OFFSET      0x0b
#define SPI_CTL_XCH_OFFSET      0x0a
#define SPI_CTL_RFRST_OFFSET    0x09
#define SPI_CTL_TFRST_OFFSET    0x08
#define SPI_CTL_SSCTL_OFFSET    0x07
#define SPI_CTL_LMTF_OFFSET     0x06
#define SPI_CTL_TBW_OFFSET      0x05
#define SPI_CTL_SSPOL_OFFSET    0x04
#define SPI_CTL_POL_OFFSET      0x03
#define SPI_CTL_PHA_OFFSET      0x02
#define SPI_CTL_MODE_OFFSET     0x01
#define SPI_CTL_EN_OFFSET       0x00



#define SPI_DDMA_PARM   0x7070707

#define SUNXI_SPI0_BASE 0x01C68000
#define SUNXI_SPI1_BASE 0x01C69000

//#define SPI_RX_DATA      (SUNXI_SPI0_BASE + 0x00)        //SPI Rx DATA Register
//#define SPI_TX_DATA      (SUNXI_SPI0_BASE + 0x04)        //SPI Tx Data Register
//#define SPI_CONTROL      (SUNXI_SPI0_BASE + 0x08)        //SPI Control Register
//#define SPI_INTCTRL      (SUNXI_SPI0_BASE + 0x0c)        //SPI Interrupt Control Resgiter
//#define SPI_INTSTAT      (SUNXI_SPI0_BASE + 0x10)        //SPI Interrupt Status Register
//#define SPI_DMACTRL      (SUNXI_SPI0_BASE + 0x14)        //SPI DMA Control Register
//#define SPI_WATCLCK      (SUNXI_SPI0_BASE + 0x18)        //SPI Wait Clock Control Register
//#define SPI_CLCKRATE     (SUNXI_SPI0_BASE + 0x1c)        //SPI Clock Rate Control Register
//#define SPI_BURSTCNT     (SUNXI_SPI0_BASE + 0x20)        //SPI Burst Counter Register
//#define SPI_TRANSCNT     (SUNXI_SPI0_BASE + 0x24)        //SPI Transmit Counter Register
//#define SPI_FIFOSTAT     (SUNXI_SPI0_BASE + 0x28)        //SPI FIFO Status Register

#define SPI_RX_DATA      (SUNXI_SPI0_BASE + 0x300)        //SPI Rx DATA Register
#define SPI_TX_DATA      (SUNXI_SPI0_BASE + 0x200)        //SPI Tx Data Register

#define SPI_CTL          (SUNXI_SPI0_BASE + 0x04)        //SPI Control Register
#define SPI_CONTROL      (SUNXI_SPI0_BASE + 0x08)        //SPI Control Register

#define SPI_INTCTRL      (SUNXI_SPI0_BASE + 0x10)        //SPI Interrupt Control Resgiter
#define SPI_INTSTAT      (SUNXI_SPI0_BASE + 0x14)        //SPI Interrupt Status Register

#define SPI_DMACTRL      (SUNXI_SPI0_BASE + 0x18)        //SPI DMA Control Register******
#define SPI_WATCLCK      (SUNXI_SPI0_BASE + 0x20)        //SPI Wait Clock Control Register
#define SPI_CLCKRATE     (SUNXI_SPI0_BASE + 0x24)        //SPI Clock Rate Control Register ****
#define SPI_BURSTCNT     (SUNXI_SPI0_BASE + 0x30)        //SPI Burst Counter Register
#define SPI_TRANSCNT     (SUNXI_SPI0_BASE + 0x34)        //SPI Transmit Counter Register
#define SPI_FIFOSTAT     (SUNXI_SPI0_BASE + 0x1c)        //SPI FIFO Status Register

//----------------LINUX---------------------------------------------------------------------------------
#define SPI_VER_REG		        (SUNXI_SPI0_BASE + 0x00)		/* version number register */
#define SPI_GC_REG			(SUNXI_SPI0_BASE + 0x04)		/* global control register */
#define SPI_TC_REG			(SUNXI_SPI0_BASE + 0x08) 		/* transfer control register */
#define SPI_INT_CTL_REG			(SUNXI_SPI0_BASE + 0x10) 		/* interrupt control register */
#define SPI_INT_STA_REG			(SUNXI_SPI0_BASE + 0x14) 		/* interrupt status register */
#define SPI_FIFO_CTL_REG		(SUNXI_SPI0_BASE + 0x18) 		/* fifo control register */
#define SPI_FIFO_STA_REG		(SUNXI_SPI0_BASE + 0x1C) 		/* fifo status register */
#define SPI_WAIT_CNT_REG		(SUNXI_SPI0_BASE + 0x20) 		/* wait clock counter register */
#define SPI_CLK_CTL_REG			(SUNXI_SPI0_BASE + 0x24) 		/* clock rate control register */
#define SPI_BURST_CNT_REG		(SUNXI_SPI0_BASE + 0x30) 		/* burst counter register */
#define SPI_TRANSMIT_CNT_REG	        (SUNXI_SPI0_BASE + 0x34) 		/* transmit counter register */
#define SPI_BCC_REG			(SUNXI_SPI0_BASE + 0x38) 		/* burst control counter register */
#define SPI_DMA_CTL_REG			(SUNXI_SPI0_BASE + 0x88)		/* DMA control register, only for 1639 */
#define SPI_TXDATA_REG			(SUNXI_SPI0_BASE + 0x200) 	        /* tx data register */
#define SPI_RXDATA_REG			(SUNXI_SPI0_BASE + 0x300) 	        /* rx data register */

/* SPI Global Control Register Bit Fields & Masks,defualt value:0x0000_0080 */
#define SPI_GC_EN			(0x1 <<  0) 	/* SPI module enable control 1:enable; 0:disable; default:0 */
#define SPI_GC_MODE   		        (0x1 <<  1) 	/* SPI function mode select 1:master; 0:slave; default:0 */
#define SPI_GC_TP_EN		        (0x1 <<  7) 	/* SPI transmit stop enable 1:stop transmit data when RXFIFO is full; 0:ignore RXFIFO status; default:1 */
#define SPI_GC_SRST			(0x1 << 31) 	/* soft reset, write 1 will clear SPI control, auto clear to 0 */

/* SPI Transfer Control Register Bit Fields & Masks,defualt value:0x0000_0087 */
#define SPI_TC_PHA			(0x1 <<  0)		/* SPI Clock/Data phase control,0: phase0,1: phase1;default:1 */
#define SPI_TC_POL			(0x1 <<  1)		/* SPI Clock polarity control,0:low level idle,1:high level idle;default:1 */
#define SPI_TC_SPOL	                (0x1 <<  2)  	/* SPI Chip select signal polarity control,default: 1,low effective like this:~~|_____~~ */
#define SPI_TC_SSCTL                    (0x1 <<  3)  	/* SPI chip select control,default 0:SPI_SSx remains asserted between SPI bursts,1:negate SPI_SSx between SPI bursts */
#define SPI_TC_SS_MASK		        (0x3 <<  4)		/* SPI chip select:00-SPI_SS0;01-SPI_SS1;10-SPI_SS2;11-SPI_SS3*/
#define SPI_TC_SS_OWNER		        (0x1 <<  6)		/* SS output mode select default is 0:automatic output SS;1:manual output SS */
#define SPI_TC_SS_LEVEL		        (0x1 <<  7)		/* defautl is 1:set SS to high;0:set SS to low */
#define SPI_TC_DHB			(0x1 <<  8)		/* Discard Hash Burst,default 0:receiving all spi burst in BC period 1:discard unused,fectch WTC bursts */
#define SPI_TC_DDB			(0x1 <<  9)		/* Dummy burst Type,default 0: dummy spi burst is zero;1:dummy spi burst is one */
#define SPI_TC_RPSM			(0x1 << 10) 	/* select mode for high speed write,0:normal write mode,1:rapids write mode,default 0 */
#define SPI_TC_SDC			(0x1 << 11) 	/* master sample data control, 1: delay--high speed operation;0:no delay. */
#define SPI_TC_FBS			(0x1 << 12) 	/* LSB/MSB transfer first select 0:MSB,1:LSB,default 0:MSB first */
#define SPI_TC_XCH                      (0x1 << 31) 	/* Exchange burst default 0:idle,1:start exchange;when BC is zero,this bit cleared by SPI controller*/
#define SPI_TC_SS_BIT_POS			(4)

/* SPI Interrupt Control Register Bit Fields & Masks,defualt value:0x0000_0000 */
#define SPI_INTEN_RX_RDY	(0x1 <<  0)  	/* rxFIFO Ready Interrupt Enable,---used for immediately received,0:disable;1:enable */
#define SPI_INTEN_RX_EMP	(0x1 <<  1)  	/* rxFIFO Empty Interrupt Enable ---used for IRQ received */
#define SPI_INTEN_RX_FULL	(0x1 <<  2)  	/* rxFIFO Full Interrupt Enable ---seldom used */
#define SPI_INTEN_TX_ERQ	(0x1 <<  4)  	/* txFIFO Empty Request Interrupt Enable ---seldom used */
#define SPI_INTEN_TX_EMP	(0x1 <<  5)  	/* txFIFO Empty Interrupt Enable ---used  for IRQ tx */
#define SPI_INTEN_TX_FULL	(0x1 <<  6)		/* txFIFO Full Interrupt Enable ---seldom used */
#define SPI_INTEN_RX_OVF	(0x1 <<  8)		/* rxFIFO Overflow Interrupt Enable ---used for error detect */
#define SPI_INTEN_RX_UDR        (0x1 <<  9)		/* rxFIFO Underrun Interrupt Enable ---used for error detect */
#define SPI_INTEN_TX_OVF        (0x1 << 10)		/* txFIFO Overflow Interrupt Enable ---used for error detect */
#define SPI_INTEN_TX_UDR	(0x1 << 11) 	/* txFIFO Underrun Interrupt Enable ---not happened */
#define SPI_INTEN_TC		(0x1 << 12) 	/* Transfer Completed Interrupt Enable  ---used */
#define SPI_INTEN_SSI		(0x1 << 13) 	/* SSI interrupt Enable,chip select from valid state to invalid state,for slave used only */
#define SPI_INTEN_ERR		(SPI_INTEN_TX_OVF|SPI_INTEN_RX_UDR|SPI_INTEN_RX_OVF) /* NO txFIFO underrun */
#define SPI_INTEN_MASK		(0x77|(0x3f<<8))

/* SPI Interrupt Status Register Bit Fields & Masks,defualt value:0x0000_0022 */
#define SPI_INT_STA_RX_RDY		(0x1 <<  0)		/* rxFIFO ready, 0:RX_WL < RX_TRIG_LEVEL,1:RX_WL >= RX_TRIG_LEVEL */
#define SPI_INT_STA_RX_EMP		(0x1 <<  1)		/* rxFIFO empty, this bit is set when rxFIFO is empty */
#define SPI_INT_STA_RX_FULL		(0x1 <<  2)		/* rxFIFO full, this bit is set when rxFIFO is full */
#define SPI_INT_STA_TX_RDY		(0x1 <<  4)		/* txFIFO ready, 0:TX_WL > TX_TRIG_LEVEL,1:TX_WL <= TX_TRIG_LEVEL */
#define SPI_INT_STA_TX_EMP		(0x1 <<  5)		/* txFIFO empty, this bit is set when txFIFO is empty */
#define SPI_INT_STA_TX_FULL		(0x1 <<  6)		/* txFIFO full, this bit is set when txFIFO is full */
#define SPI_INT_STA_RX_OVF		(0x1 <<  8)		/* rxFIFO overflow, when set rxFIFO has overflowed */
#define SPI_INT_STA_RX_UDR		(0x1 <<  9)		/* rxFIFO underrun, when set rxFIFO has underrun */
#define SPI_INT_STA_TX_OVF		(0x1 << 10)		/* txFIFO overflow, when set txFIFO has overflowed */
#define SPI_INT_STA_TX_UDR		(0x1 << 11)		/* fxFIFO underrun, when set txFIFO has underrun */
#define SPI_INT_STA_TC			(0x1 << 12)		/* Transfer Completed */
#define SPI_INT_STA_SSI			(0x1 << 13)		/* SS invalid interrupt, when set SS has changed from valid to invalid */
#define SPI_INT_STA_ERR			(SPI_INT_STA_TX_OVF|SPI_INT_STA_RX_UDR|SPI_INT_STA_RX_OVF) /* NO txFIFO underrun */
#define SPI_INT_STA_MASK		(0x77|(0x3f<<8))

/* SPI FIFO Control Register Bit Fields & Masks,defualt value:0x0040_0001 */
#define SPI_FIFO_CTL_RX_LEVEL	(0xFF <<  0)	/* rxFIFO reday request trigger level,default 0x1 */
#define SPI_FIFO_CTL_RX_DRQEN	(0x1  <<  8)	/* rxFIFO DMA request enable,1:enable,0:disable */
#define SPI_FIFO_CTL_RX_TESTEN	(0x1  << 14)	/* rxFIFO test mode enable,1:enable,0:disable */
#define SPI_FIFO_CTL_RX_RST	(0x1  << 15)	/* rxFIFO reset, write 1, auto clear to 0 */
#define SPI_FIFO_CTL_TX_LEVEL	(0xFF << 16)	/* txFIFO empty request trigger level,default 0x40 */
#define SPI_FIFO_CTL_TX_DRQEN	(0x1  << 24)	/* txFIFO DMA request enable,1:enable,0:disable */
#define SPI_FIFO_CTL_TX_TESTEN	(0x1  << 30)	/* txFIFO test mode enable,1:enable,0:disable */
#define SPI_FIFO_CTL_TX_RST	(0x1  << 31)	/* txFIFO reset, write 1, auto clear to 0 */
#define SPI_FIFO_CTL_DRQEN_MASK	(SPI_FIFO_CTL_TX_DRQEN|SPI_FIFO_CTL_RX_DRQEN)

/* SPI FIFO Status Register Bit Fields & Masks,defualt value:0x0000_0000 */
#define SPI_FIFO_STA_RX_CNT		(0xFF <<  0)	/* rxFIFO counter,how many bytes in rxFIFO */
#define SPI_FIFO_STA_RB_CNT		(0x7  << 12)	/* rxFIFO read buffer counter,how many bytes in rxFIFO read buffer */
#define SPI_FIFO_STA_RB_WR		(0x1  << 15)	/* rxFIFO read buffer write enable */
#define SPI_FIFO_STA_TX_CNT		(0xFF << 16)	/* txFIFO counter,how many bytes in txFIFO */
#define SPI_FIFO_STA_TB_CNT		(0x7  << 28)	/* txFIFO write buffer counter,how many bytes in txFIFO write buffer */
#define SPI_FIFO_STA_TB_WR		(0x1  << 31)	/* txFIFO write buffer write enable */
#define SPI_RXCNT_BIT_POS		(0)
#define SPI_TXCNT_BIT_POS		(16)

/* SPI Wait Clock Register Bit Fields & Masks,default value:0x0000_0000 */
#define SPI_WAIT_WCC_MASK		(0xFFFF <<  0)	/* used only in master mode: Wait Between Transactions */
#define SPI_WAIT_SWC_MASK		(0xF    << 16)	/* used only in master mode: Wait before start dual data transfer in dual SPI mode */

/* SPI Clock Control Register Bit Fields & Masks,default:0x0000_0002 */
#define SPI_CLK_CTL_CDR2		(0xFF <<  0)	/* Clock Divide Rate 2,master mode only : SPI_CLK = AHB_CLK/(2*(n+1)) */
#define SPI_CLK_CTL_CDR1		(0xF  <<  8)	/* Clock Divide Rate 1,master mode only : SPI_CLK = AHB_CLK/2^n */
#define SPI_CLK_CTL_DRS			(0x1  << 12)	/* Divide rate select,default,0:rate 1;1:rate 2 */
#define SPI_CLK_SCOPE			(SPI_CLK_CTL_CDR2+1)

/* SPI Master Burst Counter Register Bit Fields & Masks,default:0x0000_0000 */
/* master mode: when SMC = 1,BC specifies total burst number, Max length is 16Mbytes */
#define SPI_BC_CNT_MASK			(0xFFFFFF << 0) 	/* Total Burst Counter, tx length + rx length ,SMC=1 */

/* SPI Master Transmit Counter reigster default:0x0000_0000 */
#define SPI_TC_CNT_MASK			(0xFFFFFF << 0)		/* Write Transmit Counter, tx length, NOT rx length!!! */

/* SPI Master Burst Control Counter reigster Bit Fields & Masks,default:0x0000_0000 */
#define SPI_BCC_STC_MASK		(0xFFFFFF <<  0)	/* master single mode transmit counter */
#define SPI_BCC_DBC_MASK		(0xF	  << 24)	/* master dummy burst counter */
#define SPI_BCC_DUAL_MOD_RX_EN	(0x1	  << 28)	/* master dual mode RX enable */


#define SPI_PHA_ACTIVE_		    (0x01)
#define SPI_POL_ACTIVE_		    (0x02)

#define SPI_MODE_0_ACTIVE_		(0|0)
#define SPI_MODE_1_ACTIVE_		(0|SPI_PHA_ACTIVE_)
#define SPI_MODE_2_ACTIVE_		(SPI_POL_ACTIVE_|0)
#define SPI_MODE_3_ACTIVE_		(SPI_POL_ACTIVE_|SPI_PHA_ACTIVE_)

#define SPI_CS_HIGH_ACTIVE_		(0x04)
#define SPI_LSB_FIRST_ACTIVE_		(0x08)
#define SPI_DUMMY_ONE_ACTIVE_        (0x10)
#define SPI_RECEIVE_ALL_ACTIVE_      (0x20)

/* The global infor of SPI channel. */

#define SUNXI_SPI_NUM			2

#define SUNXI_SPI_DEV_NAME		"spi"
#define SUNXI_SPI_CHAN_MASK(ch)	BIT(ch)
#define SUNXI_SPI_RES_NUM		2 /* Mem and IRQ */

 //----------END OF LINUX--------------------------------------------------------------------

#define SPI_MCLK        36          //SPI SCLK frequency in MHz
#define SPI_SCLK        18          //SPI SCLK frequency in MHz


__IO u32 *SPI_CTL_reg = (__IO u32 *)(SPI_CTL);
__IO u32 *SPI_CONTROL_reg = (__IO u32 *)(SPI_CONTROL);

int   spic_init(unsigned int spi_no);
int   spic_exit(unsigned int spi_no);
int   spic_rw  (unsigned int tcnt, void* txbuf, unsigned int rcnt, void* rxbuf);


#endif

