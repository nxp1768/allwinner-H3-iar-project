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

#include "common.h"
#include "armv7.h"
#include "timer.h"
#include "sun8iw7p1.h"
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

#define CONFIG_SYS_SRAMA2_BASE           (0x44000)

void mmu2_setup(void)
{
	u32 mmu_base;
 
	u32 *page_table = (u32 *)CONFIG_SYS_SRAMA2_BASE;
 
        int i;
	u32 reg;

        arm_init_before_mmu();
        
	page_table[0] = (3 << 10) | (15 << 5) | (1 << 3) | (0 << 2) | 0x2;
	/* the front 1G of memory(treated as 4G for all) is set up as none cacheable */
	for (i = 1; i < (CONFIG_SYS_SDRAM_BASE>>20); i++)
		page_table[i] = (i << 20) | (3 << 10) | (15 << 5) | (0 << 3) | 0x2;
	/* Set up as write through and buffered(not write back) for other 3GB, rw for everyone */
	for (i = (CONFIG_SYS_SDRAM_BASE>>20); i < 4096; i++)
		page_table[i] = (i << 20) | (3 << 10) | (15 << 5) | (1 << 3) | (0 << 2) | 0x2;

	for (i = (CONFIG_SYS_SDRAM_BASE>>20)+2; i < 4096; i++)//
			page_table[i] = (i << 20) | (3 << 10) | (15 << 5) | (0 << 3) | 0x2;
	/* flush tlb */
	asm volatile("mcr p15, 0, %0, c8, c7, 0" : : "r" (0));
	/* Copy the page table address to cp15 */
 
	mmu_base = CONFIG_SYS_SRAMA2_BASE;
 
        mmu_base |= (1 << 0) | (1 << 1) | (2 << 3);
	asm volatile("mcr p15, 0, %0, c2, c0, 0"
		     : : "r" (mmu_base) : "memory");
	asm volatile("mcr p15, 0, %0, c2, c0, 1"
		     : : "r" (mmu_base) : "memory");
	/* Set the access control to all-supervisor */
	asm volatile("mcr p15, 0, %0, c3, c0, 0" : : "r" (0x55555555));			//modified, origin value is (~0)
	asm volatile("isb");
	/* and enable the mmu */
	asm volatile("mrc p15, 0, %0, c1, c0, 0	; get CR" : "=r" (reg) : : "cc");

	//__usdelay(100);
        for (i = 1; i < (1000); i++)
        {
          ;
        }
        
	reg |= 1;    //enable mmu
	asm volatile("mcr p15, 0, %0, c1, c0, 0	; set CR" : : "r" (reg) : "cc");
	asm volatile("isb");
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
void  mmu2_turn_off( void )
{
	uint reg;
	/* and disable the mmu */
	asm volatile("mrc p15, 0, %0, c1, c0, 0	; get CR" : "=r" (reg) : : "cc");
	__usdelay(100);
	reg &= ~((7<<0)|(1<<12));    //disable mmu
	asm volatile("mcr p15, 0, %0, c1, c0, 0	; set CR" : : "r" (reg) : "cc");
	asm volatile("isb");
	/*
	 * Invalidate all instruction caches to PoU.
	 * Also flushes branch target cache.
	 */
	asm volatile ("mcr p15, 0, %0, c7, c5, 0" : : "r" (0));
	/* Invalidate entire branch predictor array */
	asm volatile ("mcr p15, 0, %0, c7, c5, 6" : : "r" (0));
	/* Full system DSB - make sure that the invalidation is complete */
	CP15DSB;
	/* ISB - make sure the instruction stream sees it */
	CP15ISB;
}

u32 get_core_id(void)
{
    u32 core_id;
    asm volatile ("mrc p15, 0, %0, c0, c0,  5" : "=r" (core_id));
    return core_id & 0x3;
}

u32 get_core_cont(void)
{
   // ; Return Value In CLUSTERID Configuration Pin
    //mrc p15,0,r0,c0,c0,5 ; R0 = Multiprocessor Affinity Register (MPIDR)
    //lsr r0,8 ; R0 = Cluster ID (Bits 8..11)
    //and r0,$F
    
    u32 core_id;
    
    asm volatile ("mrc p15, 0, %0, c0, c0,  5" : "=r" (core_id));
    
    
    core_id = core_id >> 8;
    
    
    return core_id & 0xf;
}

