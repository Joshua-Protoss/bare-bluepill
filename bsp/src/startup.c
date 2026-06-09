#include "startup.h"

#define SCB_CCR (*(volatile uint32_t*)(0xe000ed14))
#define STKALIGN (1 << 9)

extern uint32_t _stack, _sidata, _edata, _bss, _ebss, _data;
int main(void);
void default_handler(void);
void reset_handler(void);
void nmi_handler(void);
void systick_handler(void);

__attribute__((section(".isr_vector")))
vector_table_t vector_table = {
    .initial_sp_value = &_stack,
    .reset = reset_handler,
    .nmi = nmi_handler,
    .hard_fault = default_handler,
    .memory_management = default_handler,
    .bus_fault = default_handler,
    .usage_fault = default_handler,
    .sv_call = default_handler,
    .debug_monitor = default_handler,
    .pend_sv = default_handler,
    .systick = systick_handler
};

void __attribute__((weak,used)) reset_handler(void){
    volatile uint32_t *src, *dest;

    // copy all data initial values in FLASH (.data section) into pointer dest
    for (src = &_sidata, dest = &_data; dest < &_edata; src++, dest++) {
        *dest = *src;
    }

    // initialize all uninitialize variable (.ebss) to zero
    while (dest < &_ebss){
        *dest++ = 0;
    }

    
    // Activate 8-byte alignment feature, needed for bluepill
    SCB_CCR |= STKALIGN;

    (void)main();
}

void default_handler(void){
    while(1);
}

void nmi_handler(void) __attribute__((weak, used, alias("default_handler")));
void systick_handler(void) __attribute__((weak, used, alias("default_handler")));


