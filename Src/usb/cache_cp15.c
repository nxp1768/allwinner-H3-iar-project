/*
 * (C) Copyright 2002
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include "common.h"
#include "system.h"
#include "sun8iw7p1.h"
#include "type.h"

#if !(defined(CONFIG_SYS_ICACHE_OFF) && defined(CONFIG_SYS_DCACHE_OFF))

#if defined(CONFIG_SYS_ARM_CACHE_WRITETHROUGH)
#define CACHE_SETUP	0x1a
#else
#define CACHE_SETUP	0x1e
#endif



void __arm_init_before_mmu(void)
{
}
void arm_init_before_mmu(void)
	__attribute__((weak, alias("__arm_init_before_mmu")));

static void cp_delay (void)
{
	volatile int i;

	/* copro seems to need some delay between reading and writing */
	for (i = 0; i < 100; i++)
		nop();
	asm volatile("" : : : "memory");
}

static inline void dram_bank_mmu_setup(int bank)
{
  /*
	u32 *page_table = (u32 *)gd->tlb_addr;
	bd_t *bd = gd->bd;
	int	i;

	printf("%s: bank: %d\n", __func__, bank);
	for (i = bd->bi_dram[bank].start >> 20;
	     i < (bd->bi_dram[bank].start + bd->bi_dram[bank].size) >> 20;
	     i++) 
        {
		page_table[i] = i << 20 | (3 << 10) | CACHE_SETUP;
	}
        */
}

/* to activate the MMU we need to set up virtual memory: use 1M areas */
static inline void mmu_setup(void)
{
	//u32 *page_table = (u32 *)gd->tlb_addr;
	u32 mmu_base;
	u32 *page_table = (u32 *)0x44000;

	int i;
	u32 reg; 
 
	/* modified by jerry */
	arm_init_before_mmu();
	page_table[0] = (3 << 10) | (15 << 5) | (1 << 3) | (1 << 2) | 0x2;
	/* the front 1G of memory(treated as 4G for all) is set up as none cacheable */
	for (i = 1; i < (CONFIG_SYS_SDRAM_BASE>>20); i++)
		page_table[i] = (i << 20) | (3 << 10) | (15 << 5) | (0 << 3) | 0x2;
	/* Set up as write through and buffered(not write back) for other 3GB, rw for everyone */
	for (i = (CONFIG_SYS_SDRAM_BASE>>20); i < 4096; i++)
		page_table[i] = (i << 20) | (3 << 10) | (15 << 5) | (1 << 3) | (1 << 2) | 0x2;
#ifdef CONFIG_NONCACHE_MEMORY
	/* prepare a space as noncachable*/
	if(!gd->malloc_noncache_start)
	{
		printf("this memory range for noncachable is invalid\n");
	}
	else
	{
		for (i = (gd->malloc_noncache_start>>20); i < (gd->malloc_noncache_start + CONFIG_NONCACHE_MEMORY_SIZE)>>20; i++)
		page_table[i] = (i << 20) | (3 << 10) | (15 << 5) | (0 << 3) | 0x2;
	}
#endif
	/* flush tlb */
	asm volatile("mcr p15, 0, %0, c8, c7, 0" : : "r" (0));
	/* Copy the page table address to cp15 */
	mmu_base = 0x44000;
	mmu_base |= (1 << 0) | (1 << 1) | (2 << 3);
	asm volatile("mcr p15, 0, %0, c2, c0, 0"
		     : : "r" (mmu_base) : "memory");
	asm volatile("mcr p15, 0, %0, c2, c0, 1"
		     : : "r" (mmu_base) : "memory");
	/* Set the access control to all-supervisor */
	asm volatile("mcr p15, 0, %0, c3, c0, 0"
		     : : "r" (0x55555555));			//modified, origin value is (~0)

	asm volatile("isb");
	/* and enable the mmu */
	reg = get_cr();	/* get control reg. */
	cp_delay();
	set_cr(reg | CR_M);
}

static int mmu_enabled(void)
{
	return get_cr() & CR_M;
}
/* cache_bit must be either CR_I or CR_C */

extern void mmu2_setup(void);
static void cache_enable(u32 cache_bit)
{
	u32 reg;
	/* The data cache is not active unless the mmu is enabled too */
	if ((cache_bit == CR_C) && !mmu_enabled())
		mmu2_setup();

	reg = get_cr();	/* get control reg. */

	cp_delay();
	set_cr(reg | cache_bit);
}

/* cache_bit must be either CR_I or CR_C */
static void cache_disable(u32 cache_bit)
{
	u32 reg;

	if (cache_bit == CR_C) {
		/* if cache isn;t enabled no need to disable */
		reg = get_cr();
		if ((reg & CR_C) != CR_C)
			return;
		/* if disabling data cache, disable mmu too */
#if defined(CONFIG_ARM_A7)
		set_cr(reg & ~cache_bit);  /* added by jerry, to avoid the cache error */
#endif
		cache_bit |= CR_M;
		flush_dcache_all();
	}
	reg = get_cr();
	cp_delay();
	set_cr(reg & ~cache_bit);
}
#endif

 
void icache_enable(void)
{
	cache_enable(CR_I);
}

void icache_disable(void)
{
	cache_disable(CR_I);
}

int icache_status(void)
{
	return (get_cr() & CR_I) != 0;
}
 
 
void dcache_enable(void)
{
	cache_enable(CR_C);
}

void dcache_disable(void)
{
	cache_disable(CR_C);
}

int dcache_status(void)
{
	return (get_cr() & CR_C) != 0;
}

 
