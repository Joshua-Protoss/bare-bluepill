#include "startup.h"
#include "nvic.h"

#define SCB_CCR (*(volatile uint32_t*)(0xe000ed14))
#define STKALIGN (1 << 9)

extern uint32_t _stack, _sidata, _edata, _bss, _ebss, _data;
int main(void);
void default_handler(void);
void reset_handler(void);
void nmi_handler(void);
void hard_fault_handler(void);
void bus_fault_handler(void);
void systick_handler(void);
void dma1_channel1_isr(void);
void dma1_channel2_isr(void);
void dma1_channel3_isr(void);
void dma1_channel4_isr(void);
void dma1_channel5_isr(void);
void dma1_channel6_isr(void);
void dma1_channel7_isr(void);
void usart1_isr(void);
void usart2_isr(void);
void usart3_isr(void);

__attribute__((section(".isr_vector")))
vector_table_t vector_table = {
    .initial_sp_value = &_stack,
    .reset = reset_handler,
    .nmi = nmi_handler,
    .hard_fault = hard_fault_handler,
    .memory_management = default_handler,
    .bus_fault = bus_fault_handler,
    .usage_fault = default_handler,
    .sv_call = default_handler,
    .debug_monitor = default_handler,
    .pend_sv = default_handler,
    .systick = systick_handler,
    .irq_handler = {
        [NVIC_DMA1_CHANNEL1_IRQ] = dma1_channel1_isr,           // DMA1 Channel1 = IRQ 11 ADC1
        [NVIC_DMA1_CHANNEL2_IRQ] = dma1_channel2_isr,           // DMA1 Channel2 = IRQ 12 SPI1_RX   
        [NVIC_DMA1_CHANNEL3_IRQ] = dma1_channel3_isr,           // DMA1 Channel3 = IRQ 13 SPI1_TX  
        [NVIC_DMA1_CHANNEL4_IRQ] = dma1_channel4_isr,           // DMA1 Channel4 = IRQ 14 USART1_TX 
        [NVIC_DMA1_CHANNEL5_IRQ] = dma1_channel5_isr,           // DMA1 Channel5 = IRQ 15 USART1_RX
        [NVIC_DMA1_CHANNEL6_IRQ] = dma1_channel6_isr,           // DMA1 Channel6 = IRQ 16 USART2_RX 
        [NVIC_DMA1_CHANNEL7_IRQ] = dma1_channel7_isr,           // DMA1 Channel7 = IRQ 17 USART2_TX
        [NVIC_USART1_IRQ] = usart1_isr,                         // USART1 = IRQ 37
        [NVIC_USART2_IRQ] = usart2_isr,                         // USART2 = IRQ 38
        [NVIC_USART3_IRQ] = usart3_isr,                         // USART3 = IRQ 39
    }
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

void bus_fault_handler(void) {
    // These variables are read in debugger, not used in code
    // The volatile prevents compiler from optimizing them away
    volatile uint32_t cfsr = *(volatile uint32_t*)0xE000ED28;
    volatile uint32_t bfar = *(volatile uint32_t*)0xE000ED38;
    
    // Suppress "unused variable" warnings:
    (void)cfsr;
    (void)bfar;
    
    while(1) {
        // Breakpoint here for debugging
    }
}

void hard_fault_handler(void) __attribute__((weak, used, alias("default_handler")));
void dma1_channel1_isr(void) __attribute__((weak, used, alias("default_handler")));
void dma1_channel2_isr(void) __attribute__((weak, used, alias("default_handler")));
void dma1_channel3_isr(void) __attribute__((weak, used, alias("default_handler")));
void dma1_channel4_isr(void) __attribute__((weak, used, alias("default_handler")));
void dma1_channel5_isr(void) __attribute__((weak, used, alias("default_handler")));
void dma1_channel6_isr(void) __attribute__((weak, used, alias("default_handler")));
void dma1_channel7_isr(void) __attribute__((weak, used, alias("default_handler")));
void usart1_isr(void) __attribute__((weak, used, alias("default_handler")));
void usart2_isr(void) __attribute__((weak, used, alias("default_handler")));
void usart3_isr(void) __attribute__((weak, used, alias("default_handler")));
void nmi_handler(void) __attribute__((weak, used, alias("default_handler")));
void systick_handler(void) __attribute__((weak, used, alias("default_handler")));


