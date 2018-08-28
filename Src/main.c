/* Includes ------------------------------------------------------------------*/


#include "main.h"
#include "type.h"
#include "timer.h"
#include "uart.h"
#include "clock.h"

#include "gTouch.h"

u8 buf_t[50];
volatile u32 id = 0;


int main(void)
{
   sunxi_clock_set_corepll(1200, 0);
    
   cpu_init();

    icache_enable();
    dcache_enable();  
   // icache_disable();
   // dcache_disable();
    
   id =  get_cpu_id();
   id =  get_core_id();
   id =  get_core_cont();
    
    timer_init();
    test_timer();
    enable_interrupts();
    get_dram_type_by_gpio();

    sunxi_serial_test();

    spi_test();

    EINT_A6_config();

    FastInit();

    while (1)
    {
        ;  //__usdelay(100);
    }
}



/***************************END OF FILE****/
